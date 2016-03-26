#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include <stdint.h>
#include <map>
#include "frame.h"
#include "transport.h"
#include "terminal.h"
#include "string"
#include <random>

class Client : Terminal {
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
    MQTT_ERROR sendMessage(Message* m);
    MQTT_ERROR getUsablePacketID(uint16_t* id);
    MQTT_ERROR ackMessage(uint16_t pID);
    MQTT_ERROR connect(const std::string addr, int port, bool cleanSession);
    MQTT_ERROR publish(const std::string topic, const std::string data, uint8_t qos, bool retain);
    MQTT_ERROR subscribe(std::vector<SubscribeTopic*> topics);
    MQTT_ERROR unsubscribe(std::vector<std::string> topics);
    MQTT_ERROR redelivery();
    void setPreviousSession(Client* ps);
    MQTT_ERROR recvConnectMessage(ConnectMessage* m);
    MQTT_ERROR recvConnackMessage(ConnackMessage* m);
    MQTT_ERROR recvPublishMessage(PublishMessage* m);
    MQTT_ERROR recvPubackMessage(PubackMessage* m);
    MQTT_ERROR recvPubrecMessage(PubrecMessage* m);
    MQTT_ERROR recvPubrelMessage(PubrelMessage* m);
    MQTT_ERROR recvPubcompMessage(PubcompMessage* m);
    MQTT_ERROR recvSubscribeMessage(SubscribeMessage* m);
    MQTT_ERROR recvSubackMessage(SubackMessage* m);
    MQTT_ERROR recvUnsubscribeMessage(UnsubscribeMessage* m);
    MQTT_ERROR recvUnsubackMessage(UnsubackMessage* m);
    MQTT_ERROR recvPingreqMessage(PingreqMessage* m);
    MQTT_ERROR recvPingrespMessage(PingrespMessage* m);
    MQTT_ERROR recvDisconnectMessage(DisconnectMessage* m);
};


#endif //MQTT_CLIENT_H_
