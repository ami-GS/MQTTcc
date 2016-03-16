#ifndef MQTT_TERMINAL_H_
#define MQTT_TERMINAL_H_

#include "frame.h"

class Terminal {
public:
    Terminal();
    virtual ~Terminal();
    virtual int recvConnectMessage(ConnectMessage* m) = 0;
    virtual int recvConnackMessage(ConnackMessage* m) = 0;
    virtual int recvPublishMessage(PublishMessage* m) = 0;
    virtual int recvPubackMessage(PubackMessage* m) = 0;
    virtual int recvPubrecMessage(PubrecMessage* m) = 0;
    virtual int recvPubrelMessage(PubrelMessage* m) = 0;
    virtual int recvPubcompMessage(PubcompMessage* m) = 0;
    virtual int recvSubscribeMessage(SubscribeMessage* m) = 0;
    virtual int recvSubackMessage(SubackMessage* m) = 0;
    virtual int recvUnsubscribeMessage(UnsubscribeMessage* m) = 0;
    virtual int recvUnsubackMessage(UnsubackMessage* m) = 0;
    virtual int recvPingreqMessage(PingreqMessage* m) = 0;
    virtual int recvPingrespMessage(PingrespMessage* m) = 0;
    virtual int recvDisconnectMessage(DisconnectMessage* m) = 0;
};

#endif // MQTT_TERMINAL_H_
