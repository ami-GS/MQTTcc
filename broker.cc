#include "broker.h"
#include "frame.h"
#include <sstream>
#include <thread>
#include <sys/socket.h>
#include "unistd.h"

Broker::Broker() {
    this->topicRoot = new TopicNode("", "");
}

Broker::~Broker() {
    delete this->topicRoot;
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
        bc->readThread = &t;
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

void Broker::ApplyDummyClientID(std::string* id) {
    std::stringstream ss;
    ss << "DummyClientID" << this->clients.size() + 1;
    *id = ss.str();
    return;
}


BrokerSideClient::BrokerSideClient(Transport* ct, Broker* b) : broker(b), Terminal("", NULL, 0, NULL) {
    this->ct = ct;
}

BrokerSideClient::~BrokerSideClient() {}

MQTT_ERROR  BrokerSideClient::disconnectProcessing() {
    MQTT_ERROR err = NO_ERROR;
    if (this->will != NULL) {
        if (this->will->retain) {
            this->broker->topicRoot->applyRetain(will->topic, will->qos, will->message, err);
            if (err != NO_ERROR) {
                return err;
                }
        }
        std::vector<TopicNode*> nodes = this->broker->topicRoot->getTopicNode(will->topic, true, err);
        if (err != NO_ERROR) {
            return err;
        }

        for (std::map<std::string, uint8_t>::iterator it = nodes[0]->subscribers.begin(); it != nodes[0]->subscribers.end(); it++) {
            BrokerSideClient* subscriber = broker->clients[it->first];
            this->broker->checkQoSAndPublish(subscriber, this->will->qos, it->second, this->will->retain, this->will->topic, this->will->message);
        }
    }
    if (this->isConnecting) {
        // stop keepalive timer
        if (this->cleanSession) {
            delete this->broker->clients[ID];
        }
    }
    err = this->disconnectBase();
    return err;
}

void expirationTimer(BrokerSideClient* bc, int tID) {
    usleep(bc->keepAlive);
    bool gotPing = bc->threads[tID];
    if (!gotPing) {
        // TODO : show some error
        bc->disconnectProcessing();
    }
    return;
}

void BrokerSideClient::setPreviousSession(BrokerSideClient* ps) {
    this->subTopics = ps->subTopics;
    this->packetIDMap = ps->packetIDMap;
    this->cleanSession = ps->cleanSession;
    this->will = ps->will;
    this->user = ps->user;
    this->keepAlive = ps->keepAlive;
}

MQTT_ERROR BrokerSideClient::recvConnectMessage(ConnectMessage* m) {
    MQTT_ERROR err = NO_ERROR;
    if (m->protocol.name != MQTT_3_1_1.name) {
        return INVALID_PROTOCOL_NAME;
    }
    if (m->protocol.level != MQTT_3_1_1.level) {
        this->sendMessage(new ConnackMessage(false, CONNECT_UNNACCEPTABLE_PROTOCOL_VERSION));
        return INVALID_PROTOCOL_NAME;
    }
    std::map<std::string, BrokerSideClient*>::iterator bc = this->broker->clients.find(m->clientID);
    if (bc != this->broker->clients.end() && bc->second->isConnecting) {
        this->sendMessage(new ConnackMessage(false, CONNECT_IDENTIFIER_REJECTED));
        return CLIENT_ID_IS_USED_ALREADY;
    }
    bool cs = (ConnectFlag)(m->flags&CLEANSESSION_FLAG) == CLEANSESSION_FLAG;
    if (bc != this->broker->clients.end() && !this->cleanSession) {
        this->setPreviousSession(bc->second);
    } else if (!cs && m->clientID.size() == 0) {
        this->sendMessage(new ConnackMessage(false, CONNECT_IDENTIFIER_REJECTED));
        return CLEANSESSION_MUST_BE_TRUE;
    }

    bool sessionPresent = bc != this->broker->clients.end();
    if (cs || !sessionPresent) {
        // set torelant Duration
        if (m->clientID.size() == 0) {
            this->broker->ApplyDummyClientID(&(m->clientID));
        }
        this->ID = m->clientID;
        this->user = m->user;
        this->will = m->will;
        this->keepAlive = m->keepAlive;
        this->cleanSession = cs;
        sessionPresent = false;
    }
    this->broker->clients[m->clientID] = this;

    if ((ConnectFlag)(m->flags&WILL_FLAG) == WILL_FLAG) {
        this->will = m->will;
    } else {

    }
    if (m->keepAlive != 0) {
        // TODO : bad way
        this->threadIdx = 0;
        std::thread t = std::thread(expirationTimer, this, this->threadIdx);
        this->threads[this->threadIdx] = false;
        t.join();
    }
    this->isConnecting = true;
    err = this->sendMessage(new ConnackMessage(sessionPresent, CONNECT_ACCEPTED));
    err = this->redelivery();
    return err;

}
MQTT_ERROR BrokerSideClient::recvConnackMessage(ConnackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPublishMessage(PublishMessage* m) {
    if (m->fh->dup) {
        // re-delivered
    } else {
        // first time delivery
    }

    MQTT_ERROR err = NO_ERROR;
    if (m->fh->retain) {
        std::string data = m->payload;
        if (m->fh->qos == 0 && data.size() > 0) {
            data = "";
        }
        this->broker->topicRoot->applyRetain(m->topicName, m->fh->qos, data, err);
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
        BrokerSideClient* subscriber = this->broker->clients[it->first];
        err = this->broker->checkQoSAndPublish(subscriber, m->fh->qos, it->second, false, m->topicName, m->payload);
        }

    switch (m->fh->qos) {
    case 0:
        if (m->fh->packetID != 0){
            return PACKET_ID_SHOULD_BE_ZERO;
        }
    case 1:
        this->sendMessage(new PubackMessage(m->fh->packetID));
    case 2:
        this->sendMessage(new PubrecMessage(m->fh->packetID));
    }

    return err;
}



MQTT_ERROR BrokerSideClient::recvPubackMessage(PubackMessage* m) {
    if (m->fh->packetID > 0) {
        return this->ackMessage(m->fh->packetID);
    }
    return NO_ERROR;
}

MQTT_ERROR BrokerSideClient::recvPubrecMessage(PubrecMessage* m) {
    MQTT_ERROR err = this->ackMessage(m->fh->packetID);
    if (err < 0) {
        return err;
    }
    err = this->sendMessage(new PubrelMessage(m->fh->packetID));
    return err;
}

MQTT_ERROR BrokerSideClient::recvPubrelMessage(PubrelMessage* m) {
    MQTT_ERROR err = this->ackMessage(m->fh->packetID);
    if (err < 0) {
        return err;
    }
    err = this->sendMessage(new PubcompMessage(m->fh->packetID));
    return err;
}

MQTT_ERROR BrokerSideClient::recvPubcompMessage(PubcompMessage* m) {
    return this->ackMessage(m->fh->packetID);
}


MQTT_ERROR BrokerSideClient::recvSubscribeMessage(SubscribeMessage* m) {
    std::vector<SubackCode> returnCodes;

    MQTT_ERROR err; // this sould be duplicate?
    for (std::vector<SubscribeTopic*>::iterator it = m->subTopics.begin(); it != m->subTopics.end(); it++) {
        std::vector<TopicNode*> nodes = this->broker->topicRoot->getTopicNode((*it)->topic, true, err);
        SubackCode code = (SubackCode)(*it)->qos;

        if (err != NO_ERROR) {
            code = FAILURE;
        } else {
            for (std::vector<TopicNode*>::iterator nIt = nodes.begin(); nIt != nodes.end(); nIt++) {
                (*nIt)->subscribers[ID] = (*it)->qos;
                this->subTopics[(*it)->topic] = (*it)->qos;
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
    return this->sendMessage(new SubackMessage(m->fh->packetID, returnCodes));
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

    this->sendMessage(new UnsubackMessage(m->fh->packetID));
    return err;
}

MQTT_ERROR BrokerSideClient::recvUnsubackMessage(UnsubackMessage* m) {return INVALID_MESSAGE_CAME;}
MQTT_ERROR BrokerSideClient::recvPingreqMessage(PingreqMessage* m) {
    this->sendMessage(new PingrespMessage());
    if (this->keepAlive != 0) {
        this->threads[this->threadIdx++] = true; // got ping;
        std::thread t = std::thread(expirationTimer, this, this->threadIdx);
        t.join();
        this->threads[this->threadIdx] = false;
    }
    return NO_ERROR;

}
MQTT_ERROR BrokerSideClient::recvPingrespMessage(PingrespMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR BrokerSideClient::recvDisconnectMessage(DisconnectMessage* m) {
    will = NULL;
    MQTT_ERROR err = this->disconnectProcessing();
    return err;
}
