#include "client.h"

Client::Client(const std::string id, const User* user, uint16_t keepAlive, const Will* will) : ID(id), user(user), keepAlive(keepAlive), will(will), isConnecting(false) {}

Client::~Client() {
    delete user;
    delete will;
}

int64_t Client::sendMessage(Message* m) {
    if (!isConnecting) {
        return -1; // not connecting
    }
    uint16_t packetID = m->fh->PacketID;
    if (packetIDMap.find(packetID) != packetIDMap.end()) {
        return -1; // packet ID has already used
    }
    int64_t len = ct->sendMessage(m);
    if (len != -1) {
        if (m->fh->Type == PUBLISH_MESSAGE_TYPE) {
            if (packetID > 0) {
                packetIDMap[packetID] = m;
            }
        } else if (m->fh->Type == PUBREC_MESSAGE_TYPE || m->fh->Type == SUBSCRIBE_MESSAGE_TYPE || m->fh->Type == UNSUBSCRIBE_MESSAGE_TYPE || m->fh->Type == PUBREL_MESSAGE_TYPE) {
            if (packetID == 0) {
                return -1; // packet id should not be zero
            }
            packetIDMap[packetID] = m;
        }
    }
    return len;
}
    
int Client::ackMessage(uint16_t pID) {
    if (packetIDMap.find(pID) == packetIDMap.end()) {
        return -1; // packet id does not exist
    }
    packetIDMap.erase(pID);
    return 1;
}

int Client::connect(const std::string addr, int port, bool cleanSession) {
    if (ID.size() == 0 && !cleanSession) {
        return -1; // clean session must be ture
    }

    ct = new Transport(addr, port);
    ct->sendMessage(new ConnectMessage(keepAlive, ID, cleanSession, will, user));
    return 1;
}
