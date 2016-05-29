#include "broker.h"
#include "frame.h"
#include <sstream>
#include <thread>
#include <sys/socket.h>

Broker::Broker() {
    topicRoot = new TopicNode("", "");
}

Broker::~Broker() {
    delete topicRoot;
}

MQTT_ERROR Broker::Start() {
    struct sockaddr_in addr;
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = 8883;
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);
    while (true) {
        struct sockaddr_in client;
        unsigned int len = sizeof(client);
        int sock = accept(listener, (struct sockaddr *)&client, &len);
        BrokerSideClient* bc = new BrokerSideClient(new Transport(sock, &client), this);
        std::thread t(readLoop, bc);
        t.join();
    }

    return NO_ERROR;
}

MQTT_ERROR Broker::checkQoSAndPublish(BrokerSideClient* requestClient, uint8_t publisherQoS, uint8_t requestedQoS, bool retain, std::string topic, std::string message) {
    uint16_t id = 0;
    MQTT_ERROR err = NO_ERROR;
    uint8_t qos = publisherQoS;
    if (requestedQoS < publisherQoS) {
        // QoS downgrade
        qos = requestedQoS;
    }
    if (qos > 0) {
        err = requestClient->getUsablePacketID(&id);
        if (err != NO_ERROR) {
            return err;
        }
    }
    err = requestClient->sendMessage(new PublishMessage(false, qos, retain, id, topic, message));
    return err;
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
MQTT_ERROR BrokerSideClient::recvPublishMessage(PublishMessage* m) {
    if (m->fh->Dup) {
        // re-delivered
    } else {
        // first time delivery
    }

    MQTT_ERROR err = NO_ERROR;
    if (m->fh->Retain) {
        std::string data = m->payload;
        if (m->fh->QoS == 0 && data.size() > 0) {
            data = "";
        }
        broker->topicRoot->applyRetain(m->topicName, m->fh->QoS, data, err);
        if (err != NO_ERROR) {
            return err;
        }
    }

    std::vector<TopicNode*> nodes = broker->topicRoot->getTopicNode(m->topicName, true, err);
    if (err != NO_ERROR) {
        return err;
    }

    for (std::map<std::string, uint8_t>::iterator it = nodes[0]->subscribers.begin(); it != nodes[0]->subscribers.end(); it++) {
        if (nodes[0]->subscribers.find(it->first) == nodes[0]->subscribers.end()) {
            continue;
        }
        BrokerSideClient* subscriber = broker->clients[it->first];
        err = broker->checkQoSAndPublish(subscriber, m->fh->QoS, it->second, false, m->topicName, m->payload);
        }

    switch (m->fh->QoS) {
    case 0:
        if (m->fh->PacketID != 0){
            return PACKET_ID_SHOULD_BE_ZERO;
        }
    case 1:
        sendMessage(new PubackMessage(m->fh->PacketID));
    case 2:
        sendMessage(new PubrecMessage(m->fh->PacketID));
    }

    return err;
}



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
        SubackCode code = (SubackCode)(*it)->qos;

        if (err != NO_ERROR) {
            code = FAILURE;
        } else {
            for (std::vector<TopicNode*>::iterator nIt = nodes.begin(); nIt != nodes.end(); nIt++) {
                (*nIt)->subscribers[ID] = (*it)->qos;
                subTopics[(*it)->topic] = (*it)->qos;
                if ((*nIt)->retainMessage.size() > 0) {
                    err = this->broker->checkQoSAndPublish(this, (*nIt)->retainQoS, (*it)->qos, true, (*nIt)->fullPath,(*nIt)->retainMessage);
                    if (err != NO_ERROR) {
                        return err;
                    }
                }
            }
        }
        returnCodes.push_back(code);
    }
    return sendMessage(new SubackMessage(m->fh->PacketID, returnCodes));
}


MQTT_ERROR BrokerSideClient::recvSubackMessage(SubackMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR BrokerSideClient::recvUnsubscribeMessage(UnsubscribeMessage* m) {
    if (m->topics.size() == 0) {
        return PROTOCOL_VIOLATION;
    }

    MQTT_ERROR err = NO_ERROR;
    for (std::vector<std::string>::iterator it = m->topics.begin(); it != m->topics.end(); it++) {
        this->broker->topicRoot->deleteSubscriber(this->ID, *it, err);
        this->subTopics.erase(*it);
    }

    this->sendMessage(new UnsubackMessage(m->fh->PacketID));
    return err;
}

MQTT_ERROR BrokerSideClient::recvUnsubackMessage(UnsubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPingreqMessage(PingreqMessage* m) {return NO_ERROR;}
MQTT_ERROR BrokerSideClient::recvPingrespMessage(PingrespMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvDisconnectMessage(DisconnectMessage* m) {return NO_ERROR;}
