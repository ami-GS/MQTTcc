#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include <stdint.h>
#include <map>
#include "frame.h"
#include "transport.h"
#include "string"

class Client {
private:
    Transport* ct;
    bool isConnecting;
    std::string ID;
    const User* user;
    const Will* will;
    uint16_t keepAlive;
    std::map<uint16_t, Message*> packetIDMap;
    //Broker
public:
    Client(const std::string id, const User* user, uint16_t keepAlive, const Will* will);
    ~Client();
    int64_t sendMessage(Message* m);
    int ackMessage(uint16_t pID);
    int connect(const std::string addr, int port, bool cleanSession);
};


#endif //MQTT_CLIENT_H_
