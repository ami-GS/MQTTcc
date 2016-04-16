#include "terminal.h"
#include "mqttError.h"
#include <random>
#include <chrono>

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
