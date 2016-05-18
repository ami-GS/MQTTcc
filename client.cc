#include "client.h"
#include "util.h"

Client::Client(const std::string id, const User* user, uint16_t keepAlive, const Will* will) : Terminal(id, user, keepAlive, will) {
}

Client::~Client() {}

MQTT_ERROR Client::connect(const std::string addr, int port, bool cs) {
    if (ID.size() == 0 && !cleanSession) {
        return CLEANSESSION_MUST_BE_TRUE;
    }

    ct = new Transport(addr, port);
    cleanSession = cs;
    int64_t len = ct->sendMessage(new ConnectMessage(keepAlive, ID, cleanSession, will, user));
    if (len == -1) {
        //return ; // TODO: transport error?
    }
    return NO_ERROR;
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
    return sendMessage(new PublishMessage(false, qos, retain, id, topic, data));
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
    return sendMessage(new SubscribeMessage(id, topics));
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
    MQTT_ERROR err = getUsablePacketID(&id);
    if (err != NO_ERROR) {
        return err;
    }
    return sendMessage(new UnsubscribeMessage(id, topics));
}

MQTT_ERROR Client::recvConnectMessage(ConnectMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvConnackMessage(ConnackMessage* m) {
    if (m->ReturnCode != CONNECT_ACCEPTED) {
        //return m->ReturnCode;
    }

    isConnecting = true;
    if (keepAlive != 0) {
        // start ping loop
    }

    return redelivery();
}


MQTT_ERROR Client::recvPublishMessage(PublishMessage* m) {
    if (m->fh->Dup) {
        // re-delivered;
    } else {
        // first time delivery
    }

    if (m->fh->Retain) {
        // retained Message
    }

    switch (m->fh->QoS) {
    case 0:
        if (m->fh->PacketID != 0) {
            return PACKET_ID_SHOULD_BE_ZERO; // packet id should be zero
        }
    case 1:
        return sendMessage(new PubackMessage(m->fh->PacketID));
    case 2:
        return sendMessage(new PubrecMessage(m->fh->PacketID));
    }
    return NO_ERROR;
}


MQTT_ERROR Client::recvPubackMessage(PubackMessage* m) {
    if (m->fh->PacketID > 0) {
        return ackMessage(m->fh->PacketID);
    }
    return NO_ERROR;
}

MQTT_ERROR Client::recvPubrecMessage(PubrecMessage* m) {
    MQTT_ERROR err = ackMessage(m->fh->PacketID);
    if (err < 0) {
        return err;
    }
    err = sendMessage(new PubrelMessage(m->fh->PacketID));
    return err;
}
MQTT_ERROR Client::recvPubrelMessage(PubrelMessage* m) {
    MQTT_ERROR err = ackMessage(m->fh->PacketID);
    if (err < 0) {
        return err;
    }
    err = sendMessage(new PubcompMessage(m->fh->PacketID));
    return err;
}

MQTT_ERROR Client::recvPubcompMessage(PubcompMessage* m) {
    return ackMessage(m->fh->PacketID);
}

MQTT_ERROR Client::recvSubscribeMessage(SubscribeMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvSubackMessage(SubackMessage* m) {
    return ackMessage(m->fh->PacketID);
}

MQTT_ERROR Client::recvUnsubscribeMessage(UnsubscribeMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvUnsubackMessage(UnsubackMessage* m) {
    return ackMessage(m->fh->PacketID);
}

MQTT_ERROR Client::recvPingreqMessage(PingreqMessage* m) {return INVALID_MESSAGE_CAME;}

MQTT_ERROR Client::recvPingrespMessage(PingrespMessage* m) {
    // TODO: implement duration base connection management
    return NO_ERROR;
}

MQTT_ERROR Client::recvDisconnectMessage(DisconnectMessage* m) {return INVALID_MESSAGE_CAME;}

