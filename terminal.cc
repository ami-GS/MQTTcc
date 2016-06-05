#include "terminal.h"
#include "mqttError.h"
#include "frame.h"
#include <random>
#include <chrono>
#include <string.h>
#include <unistd.h>

Terminal::Terminal(const std::string id, const User* u, uint16_t keepAlive, const Will* w) : isConnecting(false), cleanSession(false), ID(id), user(u), will(w), keepAlive(keepAlive) {
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
    int64_t len = this->ct->sendMessage(m);
    if (len != -1) {
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
    return NO_ERROR;
}

MQTT_ERROR Terminal::redelivery() {
    MQTT_ERROR err;
    if (!this->cleanSession && this->packetIDMap.size() > 0) {
        for (std::map<uint16_t, Message*>::iterator itPair = this->packetIDMap.begin(); itPair != this->packetIDMap.end(); itPair++) {
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
    while (true) {
        err = c->ct->readMessage();
        if (err == NO_ERROR) {
            FixedHeader* fh = new FixedHeader();
            int len = fh->parseHeader(c->ct->readBuff, err);
            // TODO include 'm->parse' methods into constructor
            switch (fh->type) {
            case CONNECT_MESSAGE_TYPE:
            {
                ConnectMessage* m = new ConnectMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvConnectMessage(m);
            }
            case CONNACK_MESSAGE_TYPE:
            {
                ConnackMessage* m = new ConnackMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvConnackMessage(m);
            }
            case PUBLISH_MESSAGE_TYPE:
            {
                PublishMessage* m = new PublishMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvPublishMessage(m);
            }
            case PUBACK_MESSAGE_TYPE:
            {
                PubackMessage* m = new PubackMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvPubackMessage(m);
            }
            case PUBREC_MESSAGE_TYPE:
            {
                PubrecMessage* m = new PubrecMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvPubrecMessage(m);
            }
            case PUBREL_MESSAGE_TYPE:
            {
                PubrelMessage* m = new PubrelMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvPubrelMessage(m);
            }
            case PUBCOMP_MESSAGE_TYPE:
            {
                PubcompMessage* m = new PubcompMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvPubcompMessage(m);
            }
            case SUBSCRIBE_MESSAGE_TYPE:
            {
                SubscribeMessage* m = new SubscribeMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvSubscribeMessage(m);
            }
            case SUBACK_MESSAGE_TYPE:
            {
                SubackMessage* m = new SubackMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvSubackMessage(m);
            }
            case UNSUBSCRIBE_MESSAGE_TYPE:
            {
                UnsubscribeMessage* m = new UnsubscribeMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvUnsubscribeMessage(m);
            }
            case UNSUBACK_MESSAGE_TYPE:
            {
                UnsubackMessage* m = new UnsubackMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvUnsubackMessage(m);
            }
            case PINGREQ_MESSAGE_TYPE:
            {
                PingreqMessage* m = new PingreqMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvPingreqMessage(m);
            }
            case PINGRESP_MESSAGE_TYPE:
            {
                PingrespMessage* m = new PingrespMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvPingrespMessage(m);
            }
            case DISCONNECT_MESSAGE_TYPE:
            {
                DisconnectMessage* m = new DisconnectMessage(fh);
                m->parse(c->ct->readBuff, err);
                err = c->recvDisconnectMessage(m);
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
