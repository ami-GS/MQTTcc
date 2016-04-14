#include "broker.h"
#include "frame.h"
#include <sstream>


Broker::Broker() {
    topicRoot = new TopicNode("");
}

Broker::~Broker() {
    delete topicRoot;
}

MQTT_ERROR Broker::Start() {
    return NO_ERROR;
}

std::string Broker::ApplyDummyClientID() {
    std::stringstream ss;
    ss << "DummyClientID" << clients.size() + 1;
    return ss.str();
}


BrokerSideClient::BrokerSideClient(Transport* ct, Broker* b) : broker(b), Terminal("", NULL, 0, NULL) {
    this->ct = ct;
}

BrokerSideClient::~BrokerSideClient() {}

MQTT_ERROR BrokerSideClient::recvConnectMessage(ConnectMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvConnackMessage(ConnackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPublishMessage(PublishMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPubackMessage(PubackMessage* m) {
    if (m->fh->PacketID > 0) {
        return ackMessage(m->fh->PacketID);
    }
    return NO_ERROR;
}

MQTT_ERROR BrokerSideClient::recvPubrecMessage(PubrecMessage* m) {
    MQTT_ERROR err = ackMessage(m->fh->PacketID);
    if (err < 0) {
        return err;
    }
    err = sendMessage(new PubrelMessage(m->fh->PacketID));
    return err;
}

MQTT_ERROR BrokerSideClient::recvPubrelMessage(PubrelMessage* m) {
    MQTT_ERROR err = ackMessage(m->fh->PacketID);
    if (err < 0) {
        return err;
    }
    err = sendMessage(new PubcompMessage(m->fh->PacketID));
    return err;
}

MQTT_ERROR BrokerSideClient::recvPubcompMessage(PubcompMessage* m) {
    return ackMessage(m->fh->PacketID);
}


MQTT_ERROR BrokerSideClient::recvSubscribeMessage(SubscribeMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvSubackMessage(SubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvUnsubscribeMessage(UnsubscribeMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvUnsubackMessage(UnsubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPingreqMessage(PingreqMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPingrespMessage(PingrespMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvDisconnectMessage(DisconnectMessage* m) {return NO_ERROR;}
