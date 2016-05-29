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
    delete user;
    delete will;
}

MQTT_ERROR Terminal::ackMessage(uint16_t pID) {
    if (packetIDMap.find(pID) == packetIDMap.end()) {
        return PACKET_ID_DOES_NOT_EXIST; // packet id does not exist
    }
    packetIDMap.erase(pID);
    return NO_ERROR;
}

MQTT_ERROR Terminal::sendMessage(Message* m) {
    if (!isConnecting) {
        return NOT_CONNECTED;
    }
    uint16_t packetID = m->fh->PacketID;
    if (packetIDMap.find(packetID) != packetIDMap.end()) {
        return PACKET_ID_IS_USED_ALREADY;
    }
    int64_t len = ct->sendMessage(m);
    if (len != -1) {
        if (m->fh->Type == PUBLISH_MESSAGE_TYPE) {
            if (packetID > 0) {
                packetIDMap[packetID] = m;
            }
        } else if (m->fh->Type == PUBREC_MESSAGE_TYPE || m->fh->Type == SUBSCRIBE_MESSAGE_TYPE || m->fh->Type == UNSUBSCRIBE_MESSAGE_TYPE || m->fh->Type == PUBREL_MESSAGE_TYPE) {
            if (packetID == 0) {
                return PACKET_ID_SHOULD_NOT_BE_ZERO;
            }
            packetIDMap[packetID] = m;
        }
    }
    return NO_ERROR;
}

MQTT_ERROR Terminal::redelivery() {
    MQTT_ERROR err;
    if (!cleanSession && packetIDMap.size() > 0) {
        for (std::map<uint16_t, Message*>::iterator itPair = packetIDMap.begin(); itPair != packetIDMap.end(); itPair++) {
            err = sendMessage(itPair->second);
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
        exists = !(packetIDMap.find(*id) == packetIDMap.end());
    }
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
            switch (fh->Type) {
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
