#include "broker.h"
#include "frame.h"


Broker::Broker() {

}

Broker::~Broker() {

}

BrokerSideClient::BrokerSideClient(Transport* ct, Broker* b) : broker(b), Terminal("", NULL, 0, NULL) {
    this->ct = ct;
}

BrokerSideClient::~BrokerSideClient() {}

MQTT_ERROR BrokerSideClient::recvConnectMessage(ConnectMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvConnackMessage(ConnackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPublishMessage(PublishMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPubackMessage(PubackMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPubrecMessage(PubrecMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPubrelMessage(PubrelMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPubcompMessage(PubcompMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvSubscribeMessage(SubscribeMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvSubackMessage(SubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvUnsubscribeMessage(UnsubscribeMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvUnsubackMessage(UnsubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPingreqMessage(PingreqMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPingrespMessage(PingrespMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvDisconnectMessage(DisconnectMessage* m) {return NO_ERROR;}
