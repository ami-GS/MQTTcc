#ifndef MQTT_BROKER_H_
#define MQTT_BROKER_H_

#include "frame.h"
#include "terminal.h"
#include "topicTree.h"
#include <map>
#include <string.h>

class BrokerSideClient;
class Broker {
public:
    std::map<std::string, BrokerSideClient*>clients;
    TopicNode* topicRoot;
    Broker();
    ~Broker();
    MQTT_ERROR Start();
    MQTT_ERROR checkQoSAndPublish(BrokerSideClient* requestClient, uint8_t publisherQoS, uint8_t requestedQoS, bool retain, std::string topic, std::string message);
    void ApplyDummyClientID(std::string* id);
};

class BrokerSideClient : public Terminal {
private:
    Broker* broker;
    std::map<std::string, uint8_t> subTopics;
    std::thread expirationThread;
    int threadIdx;
public:
    std::map<int, bool> threads;
    BrokerSideClient(Transport* ct, Broker* broker);
    ~BrokerSideClient();
    MQTT_ERROR disconnectProcessing();
    void setPreviousSession(BrokerSideClient* ps);
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

void expirationTimer(BrokerSideClient* bc, int tID);

#endif //MQTT_BROKER_H_
