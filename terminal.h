#ifndef MQTT_TERMINAL_H_
#define MQTT_TERMINAL_H_

#include "frame.h"
#include "mqttError.h"
#include "transport.h"
#include <map>
#include <random>
#include <thread>

class Terminal {
public:
    Transport* ct;
    std::thread* readThread;
    bool isConnecting;
    bool cleanSession;
    std::string ID;
    const User* user;
    const Will* will;
    uint32_t keepAlive;
    std::map<uint16_t, Message*> packetIDMap;
    std::mt19937 mt;
    std::uniform_int_distribution<> randPacketID;
public:
    Terminal() {};
    Terminal(const std::string id, const User* user, uint32_t keepAlive, const Will* will);
    MQTT_ERROR ackMessage(uint16_t pID);
    MQTT_ERROR sendMessage(Message* m);
    MQTT_ERROR redelivery();
    MQTT_ERROR getUsablePacketID(uint16_t* id);
    MQTT_ERROR disconnectBase();
    virtual ~Terminal();
    virtual MQTT_ERROR recvConnectMessage(ConnectMessage* m) = 0;
    virtual MQTT_ERROR recvConnackMessage(ConnackMessage* m) = 0;
    virtual MQTT_ERROR recvPublishMessage(PublishMessage* m) = 0;
    virtual MQTT_ERROR recvPubackMessage(PubackMessage* m) = 0;
    virtual MQTT_ERROR recvPubrecMessage(PubrecMessage* m) = 0;
    virtual MQTT_ERROR recvPubrelMessage(PubrelMessage* m) = 0;
    virtual MQTT_ERROR recvPubcompMessage(PubcompMessage* m) = 0;
    virtual MQTT_ERROR recvSubscribeMessage(SubscribeMessage* m) = 0;
    virtual MQTT_ERROR recvSubackMessage(SubackMessage* m) = 0;
    virtual MQTT_ERROR recvUnsubscribeMessage(UnsubscribeMessage* m) = 0;
    virtual MQTT_ERROR recvUnsubackMessage(UnsubackMessage* m) = 0;
    virtual MQTT_ERROR recvPingreqMessage(PingreqMessage* m) = 0;
    virtual MQTT_ERROR recvPingrespMessage(PingrespMessage* m) = 0;
    virtual MQTT_ERROR recvDisconnectMessage(DisconnectMessage* m) = 0;
};


MQTT_ERROR readLoop(Terminal* c);


#endif // MQTT_TERMINAL_H_
