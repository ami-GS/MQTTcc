#include <stdint.h>
#include <sstream>
#include "frame.h"
#include "util.h"

int64_t GetMessage(uint8_t* wire, Message* m) {
    uint8_t* buf = wire;
    FixedHeader* fh = new FixedHeader();
    int len = fh->parseHeader(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;

    // TODO: check whether 'constractorMap[type](fh);' can be used or not
    switch (fh->Type) {
    case CONNECT_MESSAGE_TYPE:
        m = new ConnectMessage(fh);
    case CONNACK_MESSAGE_TYPE:
        m = new ConnackMessage(fh);
    case PUBLISH_MESSAGE_TYPE:
        m = new PublishMessage(fh);
    case PUBACK_MESSAGE_TYPE:
        m = new PubackMessage(fh);
    case PUBREC_MESSAGE_TYPE:
        m = new PubrecMessage(fh);
    case PUBREL_MESSAGE_TYPE:
        m = new PubrelMessage(fh);
    case PUBCOMP_MESSAGE_TYPE:
        m = new PubcompMessage(fh);
    case SUBSCRIBE_MESSAGE_TYPE:
        m = new SubscribeMessage(fh);
    case SUBACK_MESSAGE_TYPE:
        m = new SubackMessage(fh);
    case UNSUBSCRIBE_MESSAGE_TYPE:
        m = new UnsubscribeMessage(fh);
    case UNSUBACK_MESSAGE_TYPE:
        m = new UnsubackMessage(fh);
    case PINGREQ_MESSAGE_TYPE:
        m = new PingreqMessage(fh);
    case PINGRESP_MESSAGE_TYPE:
        m = new PingrespMessage(fh);
    case DISCONNECT_MESSAGE_TYPE:
        m = new DisconnectMessage(fh);
    default:
        return -1;
    }
    len = m->parse(buf);
    m->String();
    return len;
}

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


Message::Message(FixedHeader* fh) {
    this->fh = fh;
}

Message::~Message() {
    delete fh;
}

ConnectMessage::ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, struct Will* will, struct User* user) :
    KeepAlive(keepAlive), ClientID(id), CleanSession(cleanSession), Will(will), User(user), Flags(0), Protocol(MQTT_3_1_1), Message(new FixedHeader(CONNECT_MESSAGE_TYPE, false, 0, false, 0, 0)) {
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
    fh->Length = length;
}

ConnectMessage::~ConnectMessage() {
    delete Will;
    delete User;
}

int64_t ConnectMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
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

int64_t ConnectMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = 0;

    std::string name = UTF8_decode(buf, &len);
    buf += len;
    uint8_t level = *(buf++);
    if (name != MQTT_3_1_1.name || level != MQTT_3_1_1.level) {
        return -1;
    }
    this->Protocol = MQTT_3_1_1;
    this->Flags = (ConnectFlag)*(buf++);
    if ((this->Flags & RESERVED_FLAG) == RESERVED_FLAG) {
        return -1;
    }
    if ((this->Flags & USERNAME_FLAG) == USERNAME_FLAG && (this->Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        return -1;
    }
    this->KeepAlive = ((uint16_t)*(buf++) << 8);
    this->KeepAlive |= *(buf++);
    this->ClientID = UTF8_decode(buf, &len);
    buf += len;

    if ((this->Flags & WILL_FLAG) == WILL_FLAG) {
        std::string wTopic = UTF8_decode(buf, &len);
        buf += len;
        std::string wMessage = UTF8_decode(buf, &len);
        buf += len;
        bool wRetain = (this->Flags & WILL_RETAIN_FLAG) == WILL_RETAIN_FLAG;
        uint8_t wQoS = (uint8_t)((this->Flags & WILL_QOS3_FLAG) >> 3);
        this->Will = new struct Will(wTopic, wMessage, wRetain, wQoS);
    }

    if ((this->Flags & USERNAME_FLAG) == USERNAME_FLAG || (this->Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        std::string name(""), passwd("");
        if ((this->Flags & USERNAME_FLAG) == USERNAME_FLAG) {
            name = UTF8_decode(buf, &len);
            buf += len;
        }
        if ((this->Flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
            passwd = UTF8_decode(buf, &len);
            buf += len;
        }
        this->User = new struct User(name, passwd);
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
    ss << fh->String() << "Protocol=" << Protocol.name << ":" << Protocol.level << ", Flags=\n" << FlagString() << "\t, KeepAlive=" << KeepAlive << ", ClientID=" << ClientID << ", Will={" << Will->Topic << ":" << Will->Message << ", Retain=" << Will->Retain << ", QoS=" << Will->QoS << "}, UserInfo={" << User->Name << ":" << User->Passwd << "}";
    return ss.str();
}


ConnackMessage::ConnackMessage(bool sp, ConnectReturnCode code) : SessionPresent(sp), ReturnCode(code), Message(new FixedHeader(CONNACK_MESSAGE_TYPE, false, 0, false, 2, 0)) {}

int64_t ConnackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
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
    ss << fh->String() << "Session presentation=" << SessionPresent << ", Return code=" << ReturnCode;
    return ss.str();
}

int64_t ConnackMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    this->SessionPresent = (*(buf++) == 1);
    this->ReturnCode = (ConnectReturnCode)*(buf++);
    return buf - wire;
}

PublishMessage::PublishMessage(bool dup, uint8_t qos, bool retain, uint16_t id, std::string topic, std::string payload) : topicName(topic), payload(payload), Message(new FixedHeader(PUBLISH_MESSAGE_TYPE, dup, qos, retain, topic.size()+payload.size()+2, id)) {
    if (qos > 0) {
        fh->Length += 2;
    } else if (id != 0) {
        // warnning
    }
}

int64_t PublishMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    len = UTF8_encode(buf, topicName);
    if (fh->QoS > 0) {
        *(buf++) = (uint8_t)(fh->PacketID >> 8);
        *(buf++) = (uint8_t)fh->PacketID;
    }
    memcpy(buf, payload.c_str(), payload.size());
    buf += payload.size();
    return buf - wire;
}

int64_t PublishMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = 0;

    topicName = UTF8_decode(buf, &len);
    buf += len;

    if (topicName.find('#') == std::string::npos || topicName.find('+') == std::string::npos) {
        return -1;
    }


    if (fh->QoS > 0) {
        fh->PacketID = ((uint16_t)*(buf++) << 8);
        fh->PacketID |= *(buf++);
    }
    int payloadLen = fh->Length - (buf - wire - len);
    payload = std::string(buf, buf+payloadLen);

    return buf - wire;
}

std::string PublishMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID << ", Topic=" << topicName << ", Data=" << payload;
    return ss.str();
}

PubackMessage::PubackMessage(uint16_t id) : Message(new FixedHeader(PUBACK_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
    return buf - wire;
}

int64_t PubackMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);
    return buf - wire;
}

std::string PubackMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID;
    return ss.str();
}

PubrecMessage::PubrecMessage(uint16_t id) : Message(new FixedHeader(PUBREC_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubrecMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
    return buf - wire;
}

int64_t PubrecMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);
    return buf - wire;
}

std::string PubrecMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID;
    return ss.str();
}

PubrelMessage::PubrelMessage(uint16_t id) : Message(new FixedHeader(PUBREL_MESSAGE_TYPE, false, 1, false, 2, id)) {}

int64_t PubrelMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
    return buf - wire;
}

int64_t PubrelMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);
    return buf - wire;
}

std::string PubrelMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID;
    return ss.str();
}

PubcompMessage::PubcompMessage(uint16_t id) : Message(new FixedHeader(PUBCOMP_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubcompMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
    return buf - wire;
}

int64_t PubcompMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);
    return buf - wire;
}

std::string PubcompMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID;
    return ss.str();
}

SubscribeMessage::SubscribeMessage(uint16_t id, std::vector<SubscribeTopic*> topics, int tN) : subTopics(topics), topicNum(tN), Message(new FixedHeader(SUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2+tN, id)) {
    for (int i = 0; i < topicNum; i++) {
        fh->Length += topics[i]->topic.size();
    }
}

SubscribeMessage::~SubscribeMessage() {
    for (int i = 0; i < topicNum; i++) {
        delete subTopics[i];
    }
}

int64_t SubscribeMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
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

int64_t SubscribeMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = 0;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);

    for (int i = 0; i < fh->Length-2;) {
        std::string topic = UTF8_decode(buf, &len);
        if (len == -1) {
            return -1;
        }
        buf += len;
        if (*buf == 3) {
            return -1; // QoS == 3
        } else if (*buf > 3) {
            return -1; // malformed reserved part
        }
        uint8_t qos = *(buf++) & 0x03;
        subTopics.push_back(new SubscribeTopic(topic, qos));
        topicNum++;
        i += len + 1;
    }

    return buf - wire;
}

std::string SubscribeMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID << "\n";
    for (int i = 0; i < topicNum; i++) {
        ss << "\t" << i << ": Topic=" << subTopics[i]->topic << ", QoS=" << subTopics[i]->qos << "\n";
    }

    return ss.str();
}

SubackMessage::SubackMessage(uint16_t id, std::vector<SubackCode> codes, int cN) : returnCodes(codes), codeNum(cN), Message(new FixedHeader(SUBACK_MESSAGE_TYPE, false, 0, false, 2+cN, id)) {}


int64_t SubackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
    for (int i = 0; i < fh->Length-2; i++) {
        *(buf++) = returnCodes[i];
    }
    return buf - wire;
}

int64_t SubackMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);
    for (int i = 0; i < fh->Length-2; i++) {
        returnCodes.push_back((SubackCode)*(buf++));
        codeNum++;
    }

    return buf - wire;
}

std::string SubackMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID << "\n";
    for (int i = 0; i < codeNum; i++) {
        ss << "\t" << i << ": " << SubackCodeString[returnCodes[i]] << "\n";
    }

    return ss.str();
}

UnsubscribeMessage::UnsubscribeMessage(uint16_t id, std::vector<std::string> topics, int tN) : topics(topics), topicNum(tN), Message(new FixedHeader(UNSUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2 + 2*tN, id)) {
    for (int i = 0; i < topicNum; i++) {
        fh->Length += topics[i].size();
    }
}

int64_t UnsubscribeMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
    int payload_len = 2;
    for (int i = 0; payload_len < fh->Length; i++) {
        len = UTF8_encode(buf, topics[i]);
        if (len == -1) {
            return -1;
        }
        payload_len += len;
        buf += len;

    }

    return buf - wire;
}

int64_t UnsubscribeMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);
    for (int i = 0; i < fh->Length-2; i++) {
        topics.push_back(UTF8_decode(buf, &len));
        topicNum++;
        buf += len;
    }
    return buf - wire;
}

std::string UnsubscribeMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID << "\n";
    for (int i = 0; i < topicNum; i++) {
        ss << "\t" << i << ": " << topics[i] << "\n";
    }

    return ss.str();
}

UnsubackMessage::UnsubackMessage(uint16_t id) : Message(new FixedHeader(UNSUBACK_MESSAGE_TYPE, false, 0, false, 2, id)) {};

int64_t UnsubackMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->PacketID >> 8);
    *(buf++) = (uint8_t)fh->PacketID;
    return buf - wire;
}

int64_t UnsubackMessage::parse(uint8_t* wire) {
    uint8_t* buf = wire;
    fh->PacketID = ((uint16_t)*(buf++) << 8);
    fh->PacketID |= *(buf++);
    return buf - wire;
}

std::string UnsubackMessage::String() {
    std::stringstream ss;
    ss << fh->String() << "PacketID=" << fh->PacketID << "\n";
    return ss.str();
}

PingreqMessage::PingreqMessage() : Message(new FixedHeader(PINGREQ_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t PingreqMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

int64_t PingreqMessage::parse(uint8_t* wire) {
    return 0;
}

std::string PingreqMessage::String() {
    std::stringstream ss;
    ss << fh->String();
    return ss.str();
}


PingrespMessage::PingrespMessage() : Message(new FixedHeader(PINGRESP_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t PingrespMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

int64_t PingrespMessage::parse(uint8_t* wire) {
    return 0;
}

std::string PingrespMessage::String() {
    std::stringstream ss;
    ss << fh->String();
    return ss.str();
}

DisconnectMessage::DisconnectMessage() : Message(new FixedHeader(DISCONNECT_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t DisconnectMessage::GetWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->GetWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

std::string DisconnectMessage::String() {
    std::stringstream ss;
    ss << fh->String();
    return ss.str();
}

int64_t DisconnectMessage::parse(uint8_t* wire) {
    return 0;
}
