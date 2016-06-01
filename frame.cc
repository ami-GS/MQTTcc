#include <stdint.h>
#include <sstream>
#include "frame.h"
#include "util.h"
#include "mqttError.h"
#include <string.h>

FixedHeader::FixedHeader(MessageType type, bool dup, uint8_t qos, bool retain, uint32_t length, uint16_t id) :
type(type), dup(dup), qos(qos), retain(retain), length(length), packetID(id) {}

int64_t FixedHeader::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    *buf = (uint8_t)type << 4;
    if (dup) {
        *buf |= 0x08;
    }
    *buf |= (qos << 1);
    if (retain) {
        *buf |= 0x01;
    }
    int32_t len = remainEncode(++buf, length);
    return buf - wire + len;
}

int64_t FixedHeader::parseHeader(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    type = (MessageType)(*buf >> 4);
    dup = (*buf & 0x08) == 0x08;
    qos = (*buf >> 1) & 0x03;
    retain = (*buf & 0x01) == 0x01;
    // TODO: error type should be defined
    if (type == PUBREL_MESSAGE_TYPE || type == SUBSCRIBE_MESSAGE_TYPE || type == UNSUBSCRIBE_MESSAGE_TYPE) {
        if (dup || retain || qos != 1) {
            err = MALFORMED_FIXED_HEADER_RESERVED_BIT;
            return -1;
        }
    } else if (type == PUBLISH_MESSAGE_TYPE) {
        if (qos == 3) {
            err = INVALID_QOS_3;
            return -1;
        }
    } else if (dup || retain || qos != 0) {
        err = MALFORMED_FIXED_HEADER_RESERVED_BIT;
        return -1;
    }

    int len = 0;
    length = remainDecode(++buf, &len, err);
    if (err != NO_ERROR) {
        return -1;
    }

    return buf - wire + len;
}

std::string FixedHeader::getString() {
    std::stringstream ss;
    ss  << "[" << TypeString[type] << "]\ndup=" << dup << ", qos=" << qos << ", retain=" << retain << ", Remain Length=" << length << "\n";
    return ss.str();
}


Message::Message(FixedHeader* fh) {
    this->fh = fh;
}

Message::~Message() {
    delete fh;
}

ConnectMessage::ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, const struct Will* w, const struct User* u) :
    keepAlive(keepAlive), clientID(id), cleanSession(cleanSession), will(w), user(u), flags(0), protocol(MQTT_3_1_1), Message(new FixedHeader(CONNECT_MESSAGE_TYPE, false, 0, false, 0, 0)) {
    uint32_t length = 6 + protocol.name.size() + 2 + id.size();
    if (cleanSession) {
        flags |= 1; //CLEANSESSION_FLAG;
        
    }
    if (will != NULL) {
        length += 4 + will->topic.size() + will->message.size();
        flags |= 2 | (will->qos<<3);//WILL_FLAG | (will->qos<<3);
        if (will->retain) {
            flags |= WILL_RETAIN_FLAG;
        }
    }
    if (user != NULL) {
        length += 4 + user->name.size() + user->passwd.size();
        if (user->name.size() > 0) {
            flags |= 10;//USERNAME_FLAG;
        }
        if (user->passwd.size() > 0) {
            flags |= 9;//PASSWORD_FLAG;
        }
    } 
    fh->length = length;
}

ConnectMessage::~ConnectMessage() {
    delete will;
    delete user;
}

int64_t ConnectMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    len = UTF8_encode(buf, MQTT_3_1_1.name);
    buf += len;
    *(buf++) = MQTT_3_1_1.level;
    *(buf++) = flags;

    *(buf++) = (uint8_t)(keepAlive >> 8);
    *(buf++) = (uint8_t)keepAlive;
    len = UTF8_encode(buf, clientID);
    if (len == -1) {
        return -1;
    }
    buf += len;

    if ((flags & WILL_FLAG) == WILL_FLAG) {
        len = UTF8_encode(buf, will->topic);
        if (len == -1) {
            return -1;
        }
        buf += len;
        len = UTF8_encode(buf, will->message);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    if ((flags & USERNAME_FLAG) == USERNAME_FLAG) {
        len = UTF8_encode(buf, user->name);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    if ((flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        len = UTF8_encode(buf, user->passwd);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    return buf - wire;
}

int64_t ConnectMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    int64_t len = 0;

    std::string name = UTF8_decode(buf, &len);
    buf += len;
    uint8_t level = *(buf++);
    if (name != MQTT_3_1_1.name || level != MQTT_3_1_1.level) {
        return -1;
    }
    this->protocol = MQTT_3_1_1;
    this->flags = (ConnectFlag)*(buf++);
    if ((this->flags & RESERVED_FLAG) == RESERVED_FLAG) {
        err = MALFORMED_CONNECT_FLAG_BIT;
        return -1;
    }
    if ((this->flags & USERNAME_FLAG) == USERNAME_FLAG && (this->flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        err = USERNAME_DOES_NOT_EXIST_WITH_PASSWORD;
        return -1;
    }
    this->keepAlive = ((uint16_t)*(buf++) << 8);
    this->keepAlive |= *(buf++);
    this->clientID = UTF8_decode(buf, &len);
    buf += len;

    if ((this->flags & WILL_FLAG) == WILL_FLAG) {
        std::string wTopic = UTF8_decode(buf, &len);
        buf += len;
        std::string wMessage = UTF8_decode(buf, &len);
        buf += len;
        bool wRetain = (this->flags & WILL_RETAIN_FLAG) == WILL_RETAIN_FLAG;
        uint8_t wQoS = (uint8_t)((this->flags & WILL_QOS3_FLAG) >> 3);
        this->will = new struct Will(wTopic, wMessage, wRetain, wQoS);
    }

    if ((this->flags & USERNAME_FLAG) == USERNAME_FLAG || (this->flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        std::string name(""), passwd("");
        if ((this->flags & USERNAME_FLAG) == USERNAME_FLAG) {
            name = UTF8_decode(buf, &len);
            buf += len;
        }
        if ((this->flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
            passwd = UTF8_decode(buf, &len);
            buf += len;
        }
        this->user = new struct User(name, passwd);
    }

    return buf - wire;
}

std::string ConnectMessage::flagString() {
    std::string out("");
    if ((flags & CLEANSESSION_FLAG) == CLEANSESSION_FLAG) {
        out += "\tCleanSession\n";
    }
    if ((flags & WILL_FLAG) == WILL_FLAG) {
        out += "\tWillFlag\n";
    }
    switch (flags & WILL_QOS3_FLAG) {
    case WILL_QOS0_FLAG:
        out += "\tWill_QoS0\n";
    case WILL_QOS1_FLAG:
        out += "\tWill_QoS1\n";
    case WILL_QOS2_FLAG:
        out += "\tWill_QoS2\n";
    case WILL_QOS3_FLAG:
        out += "\tWill_QoS2\n";
    }
    if ((flags & WILL_RETAIN_FLAG) == WILL_RETAIN_FLAG) {
        out += "\tWillRetain\n";
    }
    if ((flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        out += "\tPassword\n";
    }
    if ((flags & USERNAME_FLAG) == USERNAME_FLAG) {
        out += "\tUsername\n";
    }
    return out;
}

std::string ConnectMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "Protocol=" << protocol.name << ":" << protocol.level << ", Flags=\n" << flagString() << "\t, KeepAlive=" << keepAlive << ", ClientID=" << clientID << ", Will={" << will->topic << ":" << will->message << ", retain=" << will->retain << ", QoS=" << will->qos << "}, UserInfo={" << user->name << ":" << user->passwd << "}";
    return ss.str();
}


ConnackMessage::ConnackMessage(bool sp, ConnectReturnCode code) : sessionPresent(sp), returnCode(code), Message(new FixedHeader(CONNACK_MESSAGE_TYPE, false, 0, false, 2, 0)) {}

int64_t ConnackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    if (sessionPresent) {
        *(buf++) = 0x01;
    }
    *(buf++) = (uint8_t)returnCode;

    return buf - wire;
}

std::string ConnackMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "Session presentation=" << sessionPresent << ", Return code=" << returnCode;
    return ss.str();
}

int64_t ConnackMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->sessionPresent = (*(buf++) == 1);
    this->returnCode = (ConnectReturnCode)*(buf++);
    return buf - wire;
}

PublishMessage::PublishMessage(bool dup, uint8_t qos, bool retain, uint16_t id, std::string topic, std::string payload) : topicName(topic), payload(payload), Message(new FixedHeader(PUBLISH_MESSAGE_TYPE, dup, qos, retain, topic.size()+payload.size()+2, id)) {
    if (qos > 0) {
        fh->length += 2;
    } else if (id != 0) {
        // warnning
    }
}

int64_t PublishMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    len = UTF8_encode(buf, topicName);
    if (fh->qos > 0) {
        *(buf++) = (uint8_t)(fh->packetID >> 8);
        *(buf++) = (uint8_t)fh->packetID;
    }
    memcpy(buf, payload.c_str(), payload.size());
    buf += payload.size();
    return buf - wire;
}

int64_t PublishMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    int64_t len = 0;

    topicName = UTF8_decode(buf, &len);
    buf += len;

    if (topicName.find('#') == std::string::npos || topicName.find('+') == std::string::npos) {
        err = WILDCARD_CHARACTERS_IN_PUBLISH;
        return -1;
    }


    if (fh->qos > 0) {
        fh->packetID = ((uint16_t)*(buf++) << 8);
        fh->packetID |= *(buf++);
    }
    int payloadLen = fh->length - (buf - wire - len);
    payload = std::string(buf, buf+payloadLen);

    return buf - wire;
}

std::string PublishMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID << ", Topic=" << topicName << ", Data=" << payload;
    return ss.str();
}

PubackMessage::PubackMessage(uint16_t id) : Message(new FixedHeader(PUBACK_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    return buf - wire;
}

int64_t PubackMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubackMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID;
    return ss.str();
}

PubrecMessage::PubrecMessage(uint16_t id) : Message(new FixedHeader(PUBREC_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubrecMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    return buf - wire;
}

int64_t PubrecMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubrecMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID;
    return ss.str();
}

PubrelMessage::PubrelMessage(uint16_t id) : Message(new FixedHeader(PUBREL_MESSAGE_TYPE, false, 1, false, 2, id)) {}

int64_t PubrelMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    return buf - wire;
}

int64_t PubrelMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubrelMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID;
    return ss.str();
}

PubcompMessage::PubcompMessage(uint16_t id) : Message(new FixedHeader(PUBCOMP_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubcompMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    return buf - wire;
}

int64_t PubcompMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubcompMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID;
    return ss.str();
}

SubscribeMessage::SubscribeMessage(uint16_t id, std::vector<SubscribeTopic*> topics) : subTopics(topics), Message(new FixedHeader(SUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2+topics.size(), id)) {
    for (std::vector<SubscribeTopic*>::iterator it = subTopics.begin(); it != subTopics.end(); it++) {
        fh->length += (*it)->topic.size();
    }
}

SubscribeMessage::~SubscribeMessage() {
    for (std::vector<SubscribeTopic*>::iterator it = subTopics.begin(); it != subTopics.end(); it++) {
        delete (*it);
    }
}

int64_t SubscribeMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    for (int i = 0; i < subTopics.size(); i++) {
        len = UTF8_encode(buf, subTopics[i]->topic);
        if (len == -1) {
            return -1;
        }
        buf += len;
        *(buf++) = subTopics[i]->qos;
    }

    return buf - wire;
}

int64_t SubscribeMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    int64_t len = 0;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);

    for (int i = 0; i < fh->length-2;) {
        std::string topic = UTF8_decode(buf, &len);
        buf += len;
        if (*buf == 3) {
            err = INVALID_QOS_3;
            return -1;
        } else if (*buf > 3) {
            err = MALFORMED_SUBSCRIBE_RESERVED_PART;
            return -1;
        }
        uint8_t qos = *(buf++) & 0x03;
        subTopics.push_back(new SubscribeTopic(topic, qos));
        i += len + 1;
    }

    return buf - wire;
}

std::string SubscribeMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID << "\n";
    for (int i = 0; i < subTopics.size(); i++) {
        ss << "\t" << i << ": Topic=" << subTopics[i]->topic << ", QoS=" << subTopics[i]->qos << "\n";
    }

    return ss.str();
}

SubackMessage::SubackMessage(uint16_t id, std::vector<SubackCode> codes) : returnCodes(codes), Message(new FixedHeader(SUBACK_MESSAGE_TYPE, false, 0, false, 2+codes.size(), id)) {}


int64_t SubackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    for (int i = 0; i < fh->length-2; i++) {
        *(buf++) = returnCodes[i];
    }
    return buf - wire;
}

int64_t SubackMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);
    for (int i = 0; i < fh->length-2; i++) {
        returnCodes.push_back((SubackCode)*(buf++));
    }

    return buf - wire;
}

std::string SubackMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID << "\n";
    for (int i = 0; i < returnCodes.size(); i++) {
        ss << "\t" << i << ": " << SubackCodeString[returnCodes[i]] << "\n";
    }

    return ss.str();
}

UnsubscribeMessage::UnsubscribeMessage(uint16_t id, std::vector<std::string> topics) : topics(topics), Message(new FixedHeader(UNSUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2 + 2*topics.size(), id)) {
    for (int i = 0; i < topics.size(); i++) {
        fh->length += topics[i].size();
    }
}

int64_t UnsubscribeMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    int payload_len = 2;
    for (int i = 0; payload_len < fh->length; i++) {
        len = UTF8_encode(buf, topics[i]);
        if (len == -1) {
            return -1;
        }
        payload_len += len;
        buf += len;

    }

    return buf - wire;
}

int64_t UnsubscribeMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    int64_t len;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);
    for (int i = 0; i < fh->length-2; i++) {
        topics.push_back(UTF8_decode(buf, &len));
        buf += len;
    }
    return buf - wire;
}

std::string UnsubscribeMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID << "\n";
    for (int i = 0; i < topics.size(); i++) {
        ss << "\t" << i << ": " << topics[i] << "\n";
    }

    return ss.str();
}

UnsubackMessage::UnsubackMessage(uint16_t id) : Message(new FixedHeader(UNSUBACK_MESSAGE_TYPE, false, 0, false, 2, id)) {};

int64_t UnsubackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(fh->packetID >> 8);
    *(buf++) = (uint8_t)fh->packetID;
    return buf - wire;
}

int64_t UnsubackMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    fh->packetID = ((uint16_t)*(buf++) << 8);
    fh->packetID |= *(buf++);
    return buf - wire;
}

std::string UnsubackMessage::getString() {
    std::stringstream ss;
    ss << fh->getString() << "packetID=" << fh->packetID << "\n";
    return ss.str();
}

PingreqMessage::PingreqMessage() : Message(new FixedHeader(PINGREQ_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t PingreqMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

int64_t PingreqMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    return 0;
}

std::string PingreqMessage::getString() {
    std::stringstream ss;
    ss << fh->getString();
    return ss.str();
}


PingrespMessage::PingrespMessage() : Message(new FixedHeader(PINGRESP_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t PingrespMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

int64_t PingrespMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    return 0;
}

std::string PingrespMessage::getString() {
    std::stringstream ss;
    ss << fh->getString();
    return ss.str();
}

DisconnectMessage::DisconnectMessage() : Message(new FixedHeader(DISCONNECT_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t DisconnectMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

std::string DisconnectMessage::getString() {
    std::stringstream ss;
    ss << fh->getString();
    return ss.str();
}

int64_t DisconnectMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    return 0;
}
