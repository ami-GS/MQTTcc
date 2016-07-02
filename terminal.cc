#include "terminal.h"
#include "mqttError.h"
#include "frame.h"
#include <random>
#include <chrono>
#include <string.h>
#include <unistd.h>

Terminal::Terminal(const std::string id, const User* u, uint32_t keepAlive, const Will* w) : isConnecting(false), cleanSession(false), ID(id), user(u), will(w), keepAlive(keepAlive*1000000) {
    std::random_device rnd;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    mt(); // TODO: apply seed
}

Terminal::~Terminal() {
    delete this->user;
    delete this->will;
}

MQTT_ERROR Terminal::ackMessage(uint16_t pID) {
    if (this->packetIDMap.find(pID) == this->packetIDMap.end()) {
        return PACKET_ID_DOES_NOT_EXIST; // packet id does not exist
    }
    Message* m = this->packetIDMap[pID];
    delete m;
    this->packetIDMap.erase(pID);
    return NO_ERROR;
}

MQTT_ERROR Terminal::sendMessage(Message* m) {
    if (!this->isConnecting) {
        return NOT_CONNECTED;
    }
    uint16_t packetID = m->fh->packetID;
    if (this->packetIDMap.find(packetID) != this->packetIDMap.end()) {
        return PACKET_ID_IS_USED_ALREADY;
    }
    MQTT_ERROR err = this->ct->sendMessage(m);
    if (err == NO_ERROR) {
        if (m->fh->type == PUBLISH_MESSAGE_TYPE) {
            if (packetID > 0) {
                this->packetIDMap[packetID] = m;
            }
        } else if (m->fh->type == PUBREC_MESSAGE_TYPE || m->fh->type == SUBSCRIBE_MESSAGE_TYPE || m->fh->type == UNSUBSCRIBE_MESSAGE_TYPE || m->fh->type == PUBREL_MESSAGE_TYPE) {
            if (packetID == 0) {
                return PACKET_ID_SHOULD_NOT_BE_ZERO;
            }
            this->packetIDMap[packetID] = m;
        }
    }
    return err;
}

MQTT_ERROR Terminal::redelivery() {
    MQTT_ERROR err;
    if (!this->cleanSession && this->packetIDMap.size() > 0) {
        for (std::map<uint16_t, Message*>::iterator itPair = this->packetIDMap.begin(); itPair != this->packetIDMap.end(); itPair++) {
            if (itPair->second->fh->type == PUBLISH_MESSAGE_TYPE) {
                itPair->second->fh->dup = true;
            }
            err = this->sendMessage(itPair->second);
            if (err != NO_ERROR) {
                return err;
            }
        }
    }
    return NO_ERROR;
}

MQTT_ERROR Terminal::getUsablePacketID(uint16_t* id) {
    bool exists = true;
    for (int trial = 0; exists; trial++) {
        if (trial == 5) {
            *id = -1;
            return FAIL_TO_SET_PACKET_ID;
        }
        *id = randPacketID(mt);
        exists = !(this->packetIDMap.find(*id) == this->packetIDMap.end());
    }
    return NO_ERROR;
}

MQTT_ERROR Terminal::disconnectBase() {
    if (this->isConnecting) {
        this->isConnecting = false;
        this->will = NULL;
    }
    close(this->ct->sock);
    return NO_ERROR;
}

MQTT_ERROR readLoop(Terminal* c) {
    MQTT_ERROR err = NO_ERROR;
    bool first = true;
    while (first || c->isConnecting) {
        first = false;
        err = c->ct->readMessage();
        if (err == NO_ERROR) {
            FixedHeader* fh = new FixedHeader();
            int len = fh->parseHeader(c->ct->readBuff, err);
            Message* m;
            switch (fh->type) {
            case CONNECT_MESSAGE_TYPE:
            {
                m = new ConnectMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvConnectMessage((ConnectMessage*)m);
                break;
            }
            case CONNACK_MESSAGE_TYPE:
            {
                m = new ConnackMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvConnackMessage((ConnackMessage*)m);
                break;
            }
            case PUBLISH_MESSAGE_TYPE:
            {
                m = new PublishMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvPublishMessage((PublishMessage*)m);
                break;
            }
            case PUBACK_MESSAGE_TYPE:
            {
                m = new PubackMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvPubackMessage((PubackMessage*)m);
                break;
            }
            case PUBREC_MESSAGE_TYPE:
            {
                m = new PubrecMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvPubrecMessage((PubrecMessage*)m);
                break;
            }
            case PUBREL_MESSAGE_TYPE:
            {
                m = new PubrelMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvPubrelMessage((PubrelMessage*)m);
                break;
            }
            case PUBCOMP_MESSAGE_TYPE:
            {
                m = new PubcompMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvPubcompMessage((PubcompMessage*)m);
                break;
            }
            case SUBSCRIBE_MESSAGE_TYPE:
            {
                m = new SubscribeMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvSubscribeMessage((SubscribeMessage*)m);
                break;
            }
            case SUBACK_MESSAGE_TYPE:
            {
                m = new SubackMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvSubackMessage((SubackMessage*)m);
                break;
            }
            case UNSUBSCRIBE_MESSAGE_TYPE:
            {
                m = new UnsubscribeMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvUnsubscribeMessage((UnsubscribeMessage*)m);
                break;
            }
            case UNSUBACK_MESSAGE_TYPE:
            {
                m = new UnsubackMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvUnsubackMessage((UnsubackMessage*)m);
                break;
            }
            case PINGREQ_MESSAGE_TYPE:
            {
                m = new PingreqMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvPingreqMessage((PingreqMessage*)m);
                break;
            }
            case PINGRESP_MESSAGE_TYPE:
            {
                m = new PingrespMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvPingrespMessage((PingrespMessage*)m);
                break;
            }
            case DISCONNECT_MESSAGE_TYPE:
            {
                m = new DisconnectMessage(fh, c->ct->readBuff+len, err);
                std::cout << "[RECV]" << m->getString() << std::endl;
                err = c->recvDisconnectMessage((DisconnectMessage*)m);
                break;
            }
            default:
                err = INVALID_MESSAGE_TYPE;
                break;
            }
            if (err != NO_ERROR) {
                return err;
            }
        }
    }
    return err;
}
