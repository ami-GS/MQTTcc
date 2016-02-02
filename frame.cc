#include <stdint.h>
#include "frame.h"
#include "util.h"

FixedHeader::FixedHeader(MessageType type, bool dup, uint8_t qos, bool retain, uint32_t length, uint16_t id) :
Type(type), Dup(dup), QoS(qos), Retain(retain), Length(length), PacketID(id) {}

int64_t FixedHeader::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    *buf = (uint8_t)Type << 4;
    if (Dup) {
        *buf |= 0x08;
    }
    *buf |= (QoS << 1);
    if (Retain) {
        *buf |= 0x01;
    }
    int32_t len = remainEncode(++buf, Length);
    return buf - wire + len;
}



ConnectMessage::ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, struct Will* will, struct User* user) :
    KeepAlive(keepAlive), ClientID(id), CleanSession(cleanSession), Will(will), User(user), Flags(0), FixedHeader(CONNECT_MESSAGE_TYPE, false, 0, false, 0, 0) {
    uint32_t length = 6 + MQTT_3_1_1.name.size() + 2 + id.size();
    if (cleanSession) {
        Flags |= 1; //CLEANSESSION_FLAG;
        
    }
    if (will != NULL) {
        length += 4 + will->Topic.size() + will->Message.size();
        Flags |= 2 | (will->QoS<<3);//WILL_FLAG | (will->QoS<<3);
        if (will->Retain) {
            Flags |= WILL_RETAIN_FLAG;
        }
    }
    if (user != NULL) {
        length += 4 + user->Name.size() + user->Passwd.size();
        if (user->Name.size() > 0) {
            Flags |= 10;//USERNAME_FLAG;
        }
        if (user->Passwd.size() > 0) {
            Flags |= 9;//PASSWORD_FLAG;
        }
    } 
    Length = length;
}

int64_t ConnectMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    len = UTF8_encode(buf, MQTT_3_1_1.name);
    buf += len;
    *(buf++) = MQTT_3_1_1.level;
    *(buf++) = Flags;

    *(buf++) = (uint8_t)(KeepAlive >> 8);
    *(buf++) = (uint8_t)KeepAlive;
    len = UTF8_encode(buf, ClientID);
    if (len == -1) {
        return -1;
    }
    buf += len;

    if ((Flags & WILL_FLAG) == WILL_FLAG) {
        len = UTF8_encode(buf, Will->Topic);
        if (len == -1) {
            return -1;
        }
        buf += len;
        len = UTF8_encode(buf, Will->Message);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    if ((Flags & USERNAME_FLAG) == USERNAME_FLAG) {
        len = UTF8_encode(buf, User->Name);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    if ((Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        len = UTF8_encode(buf, User->Passwd);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    return buf - wire;
}


ConnackMessage::ConnackMessage(bool sp, ConnectReturnCode code) : SessionPresent(sp), ReturnCode(code), FixedHeader(CONNACK_MESSAGE_TYPE, false, 0, false, 2, 0) {}

int64_t ConnackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    if (SessionPresent) {
        *(buf++) = 0x01;
    }
    *(buf++) = (uint8_t)ReturnCode;

    return buf - wire;
}

PublishMessage::PublishMessage(bool dup, uint8_t qos, bool retain, uint16_t id, std::string topic, std::string payload) : topicName(topic), payload(payload), FixedHeader(PUBLISH_MESSAGE_TYPE, dup, qos, retain, topic.size()+payload.size()+2, id) {
    if (qos > 0) {
        Length += 2;
    } else if (id != 0) {
        // warnning
    }
}

int64_t PublishMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    len = UTF8_encode(buf, topicName);
    if (QoS > 0) {
        *(buf++) = (uint8_t)(PacketID >> 8);
        *(buf++) = (uint8_t)PacketID;
    }
    memcpy(buf, payload.c_str(), payload.size());
    buf += payload.size();
    return buf - wire;
}

PubackMessage::PubackMessage(uint16_t id) : FixedHeader(PUBACK_MESSAGE_TYPE, false, 0, false, 2, id) {}

int64_t PubackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    return buf - wire;
}

PubrecMessage::PubrecMessage(uint16_t id) : FixedHeader(PUBREC_MESSAGE_TYPE, false, 0, false, 2, id) {}

int64_t PubrecMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    return buf - wire;
}

PubrelMessage::PubrelMessage(uint16_t id) : FixedHeader(PUBREL_MESSAGE_TYPE, false, 1, false, 2, id) {}

int64_t PubrelMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    return buf - wire;
}

PubcompMessage::PubcompMessage(uint16_t id) : FixedHeader(PUBCOMP_MESSAGE_TYPE, false, 0, false, 2, id) {}

int64_t PubcompMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    return buf - wire;
}

SubscribeMessage::SubscribeMessage(uint16_t id, SubscribeTopic** topics, int topicNum) : subTopics(topics), FixedHeader(SUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2+topicNum, id) {
    for (int i = 0; i < topicNum; i++) {
        Length += topics[i]->topic.size();
    }
}

int64_t SubscribeMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    int payload_len = 2;
    for (int i = 0; payload_len < Length; i++) {
        len = UTF8_encode(buf, subTopics[i]->topic);
        if (len == -1) {
            return -1;
        }
        buf += len;
        *(buf++) = subTopics[i]->qos;
        payload_len += len + 1;
    }
    return buf - wire;
}

SubackMessage::SubackMessage(uint16_t id, SubackCode* codes, int codeNum) : returnCodes(codes), FixedHeader(SUBACK_MESSAGE_TYPE, false, 0, false, 2+codeNum, id) {}


int64_t SubackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    for (int i = 0; i < Length-2; i++) {
        *(buf++) = returnCodes[i];
    }
    return buf - wire;
}

UnsubscribeMessage::UnsubscribeMessage(uint16_t id, std::string* topics, int topicNum) : topics(topics), FixedHeader(UNSUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2 + 2*topicNum, id) {
    for (int i = 0; i < topicNum; i++) {
        Length += (*topics).size();
    }
}

int64_t UnsubscribeMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    int payload_len = 2;
    for (int i = 0; payload_len < Length; i++) {
        len = UTF8_encode(buf, topics[i]);
        if (len == -1) {
            return -1;
        }
        payload_len += len;
        buf += len;

    }

    return buf - wire;
}

UnsubackMessage::UnsubackMessage(uint16_t id) : FixedHeader(UNSUBACK_MESSAGE_TYPE, false, 0, false, 2, id) {};

int64_t UnsubackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(PacketID >> 8);
    *(buf++) = (uint8_t)PacketID;
    return buf - wire;
}

PingreqMessage::PingreqMessage() : FixedHeader(PINGREQ_MESSAGE_TYPE, false, 0, false, 0, 0) {};

int64_t PingreqMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

PingrespMessage::PingrespMessage() : FixedHeader(PINGRESP_MESSAGE_TYPE, false, 0, false, 0, 0) {};

int64_t PingrespMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

DisconnectMessage::DisconnectMessage() : FixedHeader(DISCONNECT_MESSAGE_TYPE, false, 0, false, 0, 0) {};

int64_t DisconnectMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = FixedHeader::GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}
