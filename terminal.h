#ifndef MQTT_TERMINAL_H_
#define MQTT_TERMINAL_H_

#include "frame.h"
#include "mqttError.h"

class Terminal {
public:
    Terminal();
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

#endif // MQTT_TERMINAL_H_
