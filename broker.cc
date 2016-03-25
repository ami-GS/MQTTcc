#include "broker.h"
#include "frame.h"


Broker::Broker() {

}

Broker::~Broker() {

}

MQTT_ERROR Broker::recvConnectMessage(ConnectMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvConnackMessage(ConnackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR Broker::recvPublishMessage(PublishMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvPubackMessage(PubackMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvPubrecMessage(PubrecMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvPubrelMessage(PubrelMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvPubcompMessage(PubcompMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvSubscribeMessage(SubscribeMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvSubackMessage(SubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR Broker::recvUnsubscribeMessage(UnsubscribeMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvUnsubackMessage(UnsubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR Broker::recvPingreqMessage(PingreqMessage* m) {return NO_ERROR;}
MQTT_ERROR Broker::recvPingrespMessage(PingrespMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR Broker::recvDisconnectMessage(DisconnectMessage* m) {return NO_ERROR;}
