#include "broker.h"
#include "frame.h"


Broker::Broker() {

}

Broker::~Broker() {

}

int Broker::recvConnectMessage(ConnectMessage* m) {return 1;}
int Broker::recvConnackMessage(ConnackMessage* m) {return -1;}
int Broker::recvPublishMessage(PublishMessage* m) {return 1;}
int Broker::recvPubackMessage(PubackMessage* m) {return 1;}
int Broker::recvPubrecMessage(PubrecMessage* m) {return 1;}
int Broker::recvPubrelMessage(PubrelMessage* m) {return 1;}
int Broker::recvPubcompMessage(PubcompMessage* m) {return 1;}
int Broker::recvSubscribeMessage(SubscribeMessage* m) {return 1;}
int Broker::recvSubackMessage(SubackMessage* m) {return -1;}
int Broker::recvUnsubscribeMessage(UnsubscribeMessage* m) {return 1;}
int Broker::recvUnsubackMessage(UnsubackMessage* m) {return -1;}
int Broker::recvPingreqMessage(PingreqMessage* m) {return 1;}
int Broker::recvPingrespMessage(PingrespMessage* m) {return -1;}
int Broker::recvDisconnectMessage(DisconnectMessage* m) {return 1;}
