#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include <stdint.h>
#include <map>
#include "frame.h"
#include "transport.h"
#include "string"
#include <random>

class Client {
private:
    Transport* ct;
    bool isConnecting;
    bool cleanSession;
    std::string ID;
    const User* user;
    const Will* will;
    uint16_t keepAlive;
    std::map<uint16_t, Message*> packetIDMap;
    std::mt19937 mt;
    std::uniform_int_distribution<> randPacketID;
    //Broker
public:
    Client(const std::string id, const User* user, uint16_t keepAlive, const Will* will);
    ~Client();
    int64_t sendMessage(Message* m);
    int32_t getUsablePacketID();
    int ackMessage(uint16_t pID);
    int64_t connect(const std::string addr, int port, bool cleanSession);
    int64_t publish(const std::string topic, const std::string data, uint8_t qos, bool retain);
    int64_t subscribe(std::vector<SubscribeTopic*> topics);
    int64_t unsubscribe(std::vector<std::string> topics);
    int redelivery();
    void setPreviousSession(Client* ps);
};


#endif //MQTT_CLIENT_H_
