#include "client.h"
#include "util.h"
#include <thread>
#include <sys/time.h>

Client::Client(const std::string id, const User* user, uint16_t keepAlive, const Will* will) : Terminal(id, user, keepAlive, will) {
}

Client::~Client() {}

MQTT_ERROR Client::ping() {
    this->ct->sendMessage(new PingreqMessage());
    gettimeofday(&(this->timeOfPing), NULL);
    return NO_ERROR;
}

MQTT_ERROR Client::connect(const std::string addr, int port, bool cs) {
    if (this->ID.size() == 0 && !cs) {
        return CLEANSESSION_MUST_BE_TRUE;
        cs = true;
    }

    this->ct = new Transport(addr, port);
    this->cleanSession = cs;
    int64_t len = this->ct->sendMessage(new ConnectMessage(this->keepAlive, this->ID, this->cleanSession, this->will, this->user));
    if (len == -1) {
        //return ; // TODO: transport error?
    }
    std::thread t(readLoop, this);
    t.join();
    this->readThread = &t;
    return NO_ERROR;
}

MQTT_ERROR Client::disconnectProcessing() {
    MQTT_ERROR err = NO_ERROR;
    if (this->isConnecting) {
        this->isConnecting = false; // this makes readLoop stop
    }
    err = this->disconnectBase();
    return err;
}

MQTT_ERROR Client::publish(const std::string topic, const std::string data, uint8_t qos, bool retain) {
    if (qos >= 3) {
        return INVALID_QOS_3;
    }
    //if ()

    uint16_t id = 0;
    if (qos > 0) {
        MQTT_ERROR err = getUsablePacketID(&id);
        if (err != NO_ERROR) {
            return err;
        }
    }
    return this->sendMessage(new PublishMessage(false, qos, retain, id, topic, data));
}

MQTT_ERROR Client::subscribe(std::vector<SubscribeTopic*> topics) {
    uint16_t id = 0;
    MQTT_ERROR err = getUsablePacketID(&id);
    if (err != NO_ERROR) {
        return err;
    }
    for (int i = 0; i < topics.size(); i++) {
        std::vector<std::string> parts;
        split(topics[i]->topic, "/", &parts);
        for (int j = 0; i < parts.size(); i++) {
            if (parts[j][0] == '#' && j != parts.size() - 1) {
                return MULTI_LEVEL_WILDCARD_MUST_BE_ON_TAIL;
            } else if (false) {
            } // has suffix of '#' and '+'
        }
    }
    return this->sendMessage(new SubscribeMessage(id, topics));
}

MQTT_ERROR Client::unsubscribe(std::vector<std::string> topics) {
    for (int i = 0; i < topics.size(); i++) {
        std::vector<std::string> parts;
        split(topics[i], "/", &parts);
        for (int j = 0; j < parts.size(); j++) {
            if (parts[j][0] == '#' && j != parts.size() - 1) {
                return MULTI_LEVEL_WILDCARD_MUST_BE_ON_TAIL;
            } else if (false) {
            } // has suffix of '#' and '+'
        }
    }
    uint16_t id = 0;
    MQTT_ERROR err = this->getUsablePacketID(&id);
    if (err != NO_ERROR) {
        return err;
    }
    return this->sendMessage(new UnsubscribeMessage(id, topics));
}

MQTT_ERROR Client::disconnect() {
    //TODO: add more detail
    return this->sendMessage(new DisconnectMessage());
}

MQTT_ERROR Client::recvConnectMessage(ConnectMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvConnackMessage(ConnackMessage* m) {
    if (m->returnCode != CONNECT_ACCEPTED) {
        //return m->ReturnCode;
    }

    this->isConnecting = true;
    if (this->keepAlive != 0) {
        // start ping loop
    }

    return this->redelivery();
}


MQTT_ERROR Client::recvPublishMessage(PublishMessage* m) {
    if (m->fh->dup) {
        // re-delivered;
    } else {
        // first time delivery
    }

    if (m->fh->retain) {
        // retained Message
    }

    switch (m->fh->qos) {
    case 0:
        if (m->fh->packetID != 0) {
            return PACKET_ID_SHOULD_BE_ZERO; // packet id should be zero
        }
    case 1:
        return this->sendMessage(new PubackMessage(m->fh->packetID));
    case 2:
        return this->sendMessage(new PubrecMessage(m->fh->packetID));
    }
    return NO_ERROR;
}


MQTT_ERROR Client::recvPubackMessage(PubackMessage* m) {
    if (m->fh->packetID > 0) {
        return this->ackMessage(m->fh->packetID);
    }
    return NO_ERROR;
}

MQTT_ERROR Client::recvPubrecMessage(PubrecMessage* m) {
    MQTT_ERROR err = this->ackMessage(m->fh->packetID);
    if (err < 0) {
        return err;
    }
    err = this->sendMessage(new PubrelMessage(m->fh->packetID));
    return err;
}
MQTT_ERROR Client::recvPubrelMessage(PubrelMessage* m) {
    MQTT_ERROR err = this->ackMessage(m->fh->packetID);
    if (err < 0) {
        return err;
    }
    err = this->sendMessage(new PubcompMessage(m->fh->packetID));
    return err;
}

MQTT_ERROR Client::recvPubcompMessage(PubcompMessage* m) {
    return this->ackMessage(m->fh->packetID);
}

MQTT_ERROR Client::recvSubscribeMessage(SubscribeMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvSubackMessage(SubackMessage* m) {
    return this->ackMessage(m->fh->packetID);
}

MQTT_ERROR Client::recvUnsubscribeMessage(UnsubscribeMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvUnsubackMessage(UnsubackMessage* m) {
    return this->ackMessage(m->fh->packetID);
}

MQTT_ERROR Client::recvPingreqMessage(PingreqMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvPingrespMessage(PingrespMessage* m) {
    // TODO: implement duration base connection management
    struct timeval tmp;
    gettimeofday(&tmp, NULL);
    if ((tmp.tv_sec - this->timeOfPing.tv_sec)*1000000 + (tmp.tv_usec - this->timeOfPing.tv_usec) >= this->keepAlive) {
        this->sendMessage(new DisconnectMessage());
        return SERVER_TIMED_OUT;
    }
    return NO_ERROR;
}

MQTT_ERROR Client::recvDisconnectMessage(DisconnectMessage* m) {return INVALID_MESSAGE_CAME;}

