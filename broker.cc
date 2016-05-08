#include "broker.h"
#include "frame.h"
#include <sstream>


Broker::Broker() {
    topicRoot = new TopicNode("", "");
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

void BrokerSideClient::setPreviousSession(BrokerSideClient* ps) {
    subTopics = ps->subTopics;
    packetIDMap = ps->packetIDMap;
    cleanSession = ps->cleanSession;
    will = ps->will;
    user = ps->user;
    keepAlive = ps->keepAlive;
}

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


MQTT_ERROR BrokerSideClient::recvSubscribeMessage(SubscribeMessage* m) {
    std::vector<SubackCode> returnCodes;

    MQTT_ERROR err; // this sould be duplicate?
    for (std::vector<SubscribeTopic*>::iterator it = m->subTopics.begin(); it != m->subTopics.end(); it++) {
        std::vector<TopicNode*> nodes = broker->topicRoot->getTopicNode((*it)->topic, true, err);
        if (err != NO_ERROR) {
            for (std::vector<TopicNode*>::iterator nIt = nodes.begin(); nIt != nodes.end(); nIt++) {
                returnCodes.push_back(FAILURE);
            }
        } else {
            for (std::vector<TopicNode*>::iterator nIt = nodes.begin(); nIt != nodes.end(); nIt++) {
                (*nIt)->subscribers[ID] = (*it)->qos;
                returnCodes.push_back((SubackCode)(*it)->qos);
                subTopics[(*it)->topic] = (*it)->qos;
                if ((*nIt)->retainMessage.size() > 0) {
                    uint16_t pID = 0;
                    if ((*nIt)->retainQoS > 0) {
                        err = getUsablePacketID(&pID);
                        if (err != NO_ERROR) {
                            return err;
                        }
                    }
                    err = sendMessage(new PublishMessage(false, (*nIt)->retainQoS, true, pID, (*nIt)->fullPath, (*nIt)->retainMessage));
                    if (err != NO_ERROR) {
                        return err;
                    }
                }
            }
        }
    }
    return sendMessage(new SubackMessage(m->fh->PacketID, returnCodes, returnCodes.size()));
}


MQTT_ERROR BrokerSideClient::recvSubackMessage(SubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvUnsubscribeMessage(UnsubscribeMessage* m) {
    return NO_ERROR;
}

MQTT_ERROR BrokerSideClient::recvUnsubackMessage(UnsubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPingreqMessage(PingreqMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPingrespMessage(PingrespMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvDisconnectMessage(DisconnectMessage* m) {return NO_ERROR;}
