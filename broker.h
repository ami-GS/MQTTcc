#ifndef MQTT_BROKER_H_
#define MQTT_BROKER_H_

#include "frame.h"
#include "terminal.h"

class Broker : Terminal {
public:
    Broker();
    ~Broker();
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

#endif //MQTT_BROKER_H_