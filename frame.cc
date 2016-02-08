#include <stdint.h>
#include <sstream>
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

int64_t FixedHeader::parseHeader(uint8_t* wire) {
    uint8_t* buf = wire;
    Type = (MessageType)(*buf >> 4);
    Dup = (*buf & 0x80) == 0x08;
    QoS = (*buf >> 1) & 0x03;
    Retain = (*buf & 0x01) == 0x01;
    // TODO: error type should be defined
    if (Type == PUBREL_MESSAGE_TYPE || Type == SUBSCRIBE_MESSAGE_TYPE || Type == UNSUBSCRIBE_MESSAGE_TYPE) {
        if (Dup || Retain || QoS != 1) {
            return -1;
        }
    } else if (Type == PUBLISH_MESSAGE_TYPE) {
        if (QoS == 3) {
            return -1;
        }
    } else if (Dup || Retain || QoS != 0) {
        return -1;
    }

    int len = 0;
    Length = remainDecode(++buf, &len);
    if (len == -1) {
        return -1;
    }

    return buf - wire + len;
}

std::string FixedHeader::String() {
    std::stringstream ss;
    ss  << "[" << TypeString[Type] << "]\nDup=" << Dup << ", QoS=" << QoS << ", Retain=" << Retain << ", Remain Length=" << Length << "\n";
    return ss.str();
}


ConnectMessage::ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, struct Will* will, struct User* user) :
    KeepAlive(keepAlive), ClientID(id), CleanSession(cleanSession), Will(will), User(user), Flags(0), Protocol(MQTT_3_1_1), FixedHeader(CONNECT_MESSAGE_TYPE, false, 0, false, 0, 0) {
    uint32_t length = 6 + Protocol.name.size() + 2 + id.size();
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

int64_t ConnectMessage::parse(uint8_t* wire, ConnectMessage* m) {
    uint8_t* buf = wire;
    int64_t len = m->parseHeader(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;

    std::string name = UTF8_decode(buf, &len);
    buf += len;
    uint8_t level = *(buf++);
    if (name != MQTT_3_1_1.name || level != MQTT_3_1_1.level) {
        return -1;
    }
    m->Protocol = MQTT_3_1_1;
    m->Flags = (ConnectFlag)*(buf++);
    if ((m->Flags & RESERVED_FLAG) == RESERVED_FLAG) {
        return -1;
    }
    if ((m->Flags & USERNAME_FLAG) == USERNAME_FLAG && (m->Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        return -1;
    }
    m->KeepAlive |= ((uint16_t)*(buf++) << 8);
    m->KeepAlive |= *(buf++);
    m->ClientID = UTF8_decode(buf, &len);
    buf += len;

    if ((m->Flags & WILL_FLAG) == WILL_FLAG) {
        std::string wTopic = UTF8_decode(buf, &len);
        buf += len;
        std::string wMessage = UTF8_decode(buf, &len);
        buf += len;
        bool wRetain = (m->Flags & WILL_RETAIN_FLAG) == WILL_RETAIN_FLAG;
        uint8_t wQoS = (uint8_t)((m->Flags & WILL_QOS3_FLAG) >> 3);
        m->Will = new struct Will(wTopic, wMessage, wRetain, wQoS);
    }

    if ((m->Flags & USERNAME_FLAG) == USERNAME_FLAG || (m->Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        std::string name(""), passwd("");
        if ((m->Flags & USERNAME_FLAG) == USERNAME_FLAG) {
            name = UTF8_decode(buf, &len);
            buf += len;
        }
        if ((m->Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
            passwd = UTF8_decode(buf, &len);
            buf += len;
        }
        m->User = new struct User(name, passwd);
    }

    return buf - wire;
}

std::string ConnectMessage::FlagString() {
    std::string out("");
    if ((Flags & CLEANSESSION_FLAG) == CLEANSESSION_FLAG) {
        out += "\tCleanSession\n";
    }
    if ((Flags & WILL_FLAG) == WILL_FLAG) {
        out += "\tWillFlag\n";
    }
    switch (Flags & WILL_QOS3_FLAG) {
    case WILL_QOS0_FLAG:
        out += "\tWill_QoS0\n";
    case WILL_QOS1_FLAG:
        out += "\tWill_QoS1\n";
    case WILL_QOS2_FLAG:
        out += "\tWill_QoS2\n";
    case WILL_QOS3_FLAG:
        out += "\tWill_QoS2\n";
    }
    if ((Flags & WILL_RETAIN_FLAG) == WILL_RETAIN_FLAG) {
        out += "\tWillRetain\n";
    }
    if ((Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        out += "\tPassword\n";
    }
    if ((Flags & USERNAME_FLAG) == USERNAME_FLAG) {
        out += "\tUsername\n";
    }
    return out;
}

std::string ConnectMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "Protocol=" << Protocol.name << ":" << Protocol.level << ", Flags=\n" << FlagString() << "\t, KeepAlive=" << KeepAlive << ", ClientID=" << ClientID << ", Will={" << Will->Topic << ":" << Will->Message << ", Retain=" << Will->Retain << ", QoS=" << Will->QoS << "}, UserInfo={" << User->Name << ":" << User->Passwd << "}";
    return ss.str();
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

std::string ConnackMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "Session presentation=" << SessionPresent << ", Return code=" << ReturnCode;
    return ss.str();
}

int64_t ConnackMessage::parse(uint8_t* wire, ConnackMessage* m) {
    uint8_t* buf = wire;
    int64_t len =  m->parseHeader(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;

    m->SessionPresent = (*(buf++) == 1);
    m->ReturnCode = (ConnectReturnCode)*(buf++);
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

int64_t PublishMessage::parse(uint8_t* wire, PublishMessage* m) {
    uint8_t* buf = wire;
    int64_t len = m->parseHeader(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    int64_t h_len = len;

    m->topicName = UTF8_decode(buf, &len);
    buf += len;

    if (m->topicName.find('#') == std::string::npos || m->topicName.find('+') == std::string::npos) {
        return -1;
    }


    if (m->QoS > 0) {
        m->PacketID = ((uint16_t)*(buf++) << 8);
        m->PacketID |= *(buf++);
    }
    int payloadLen = m->Length - (buf - wire - h_len);
    m->payload = std::string(buf, buf+payloadLen);

    return buf - wire;
}

std::string PublishMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID << ", Topic=" << topicName << ", Data=" << payload;
    return ss.str();
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

std::string PubackMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID;
    return ss.str();
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

std::string PubrecMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID;
    return ss.str();
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

std::string PubrelMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID;
    return ss.str();
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

std::string PubcompMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID;
    return ss.str();
}

SubscribeMessage::SubscribeMessage(uint16_t id, SubscribeTopic** topics, int tN) : subTopics(topics), topicNum(tN), FixedHeader(SUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2+tN, id) {
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
    for (int i = 0; i < topicNum; i++) {
        len = UTF8_encode(buf, subTopics[i]->topic);
        if (len == -1) {
            return -1;
        }
        buf += len;
        *(buf++) = subTopics[i]->qos;
    }

    return buf - wire;
}

std::string SubscribeMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID << "\n";
    for (int i = 0; i < topicNum; i++) {
        ss << "\t" << i << ": Topic=" << subTopics[i]->topic << ", QoS=" << subTopics[i]->qos << "\n";
    }

    return ss.str();
}

SubackMessage::SubackMessage(uint16_t id, SubackCode* codes, int cN) : returnCodes(codes), codeNum(cN), FixedHeader(SUBACK_MESSAGE_TYPE, false, 0, false, 2+cN, id) {}


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

std::string SubackMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID << "\n";
    for (int i = 0; i < codeNum; i++) {
        ss << "\t" << i << ": " << SubackCodeString[returnCodes[i]] << "\n";
    }

    return ss.str();
}

UnsubscribeMessage::UnsubscribeMessage(uint16_t id, std::string* topics, int tN) : topics(topics), topicNum(tN), FixedHeader(UNSUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2 + 2*tN, id) {
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

std::string UnsubscribeMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID << "\n";
    for (int i = 0; i < topicNum; i++) {
        ss << "\t" << i << ": " << topics[i] << "\n";
    }

    return ss.str();
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

std::string UnsubackMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String() << "PacketID=" << PacketID << "\n";
    return ss.str();
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

std::string PingreqMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String();
    return ss.str();
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

std::string PingrespMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String();
    return ss.str();
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

std::string DisconnectMessage::String() {
    std::stringstream ss;
    ss << FixedHeader::String();
    return ss.str();
}
