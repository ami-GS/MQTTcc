#ifndef MQTT_BROKER_H_
#define MQTT_BROKER_H_

#include "frame.h"
#include "terminal.h"

class Broker : Terminal {
public:
    Broker();
    ~Broker();
    int recvConnectMessage(ConnectMessage* m);
    int recvConnackMessage(ConnackMessage* m);
    int recvPublishMessage(PublishMessage* m);
    int recvPubackMessage(PubackMessage* m);
    int recvPubrecMessage(PubrecMessage* m);
    int recvPubrelMessage(PubrelMessage* m);
    int recvPubcompMessage(PubcompMessage* m);
    int recvSubscribeMessage(SubscribeMessage* m);
    int recvSubackMessage(SubackMessage* m);
    int recvUnsubscribeMessage(UnsubscribeMessage* m);
    int recvUnsubackMessage(UnsubackMessage* m);
    int recvPingreqMessage(PingreqMessage* m);
    int recvPingrespMessage(PingrespMessage* m);
    int recvDisconnectMessage(DisconnectMessage* m);
};

#endif //MQTT_BROKER_H_
