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
    if (this->dup) {
        *buf |= 0x08;
    }
    *buf |= (this->qos << 1);
    if (this->retain) {
        *buf |= 0x01;
    }
    int32_t len = remainEncode(++buf, this->length);
    return buf - wire + len;
}

int64_t FixedHeader::parseHeader(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->type = (MessageType)(*buf >> 4);
    this->dup = (*buf & 0x08) == 0x08;
    this->qos = (*buf >> 1) & 0x03;
    this->retain = (*buf & 0x01) == 0x01;
    // TODO: error type should be defined
    if (type == PUBREL_MESSAGE_TYPE || type == SUBSCRIBE_MESSAGE_TYPE || type == UNSUBSCRIBE_MESSAGE_TYPE) {
        if (this->dup || this->retain || this->qos != 1) {
            err = MALFORMED_FIXED_HEADER_RESERVED_BIT;
            return -1;
        }
    } else if (this->type == PUBLISH_MESSAGE_TYPE) {
        if (this->qos == 3) {
            err = INVALID_QOS_3;
            return -1;
        }
    } else if (this->dup || this->retain || this->qos != 0) {
        err = MALFORMED_FIXED_HEADER_RESERVED_BIT;
        return -1;
    }

    int len = 0;
    this->length = remainDecode(++buf, &len, err);
    if (err != NO_ERROR) {
        return -1;
    }

    return buf - wire + len;
}

std::string FixedHeader::getString() {
    std::stringstream ss;
    ss  << "[" << TypeString[this->type] << "]\ndup=" << this->dup << ", qos=" << this->qos << ", retain=" << this->retain << ", Remain Length=" << this->length << "\n";
    return ss.str();
}


Message::Message(FixedHeader* fh) {
    this->fh = fh;
}

Message::~Message() {
    delete this->fh;
}

ConnectMessage::ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, const struct Will* w, const struct User* u) :
    keepAlive(keepAlive), clientID(id), cleanSession(cleanSession), will(w), user(u), flags(0), protocol(MQTT_3_1_1), Message(new FixedHeader(CONNECT_MESSAGE_TYPE, false, 0, false, 0, 0)) {
    std::cout << "will qos:" << this->will->qos << std::endl;
    uint32_t length = 6 + protocol.name.size() + 2 + id.size();
    if (this->cleanSession) {
        this->flags |= CLEANSESSION_FLAG;
    }
    if (will != NULL) {
        length += 4 + this->will->topic.size() + this->will->message.size();
        this->flags |= WILL_FLAG | (this->will->qos<<3);
        if (this->will->retain) {
            this->flags |= WILL_RETAIN_FLAG;
        }
    }
    if (this->user != NULL) {
        length += 4 + this->user->name.size() + this->user->passwd.size();
        if (this->user->name.size() > 0) {
            this->flags |= USERNAME_FLAG;
        }
        if (this->user->passwd.size() > 0) {
            this->flags |= PASSWORD_FLAG;
        }
    } 
    this->fh->length = length;
}

ConnectMessage::~ConnectMessage() {
    delete this->will;
    delete this->user;
}

int64_t ConnectMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    len = UTF8_encode(buf, MQTT_3_1_1.name);
    buf += len;
    *(buf++) = MQTT_3_1_1.level;
    *(buf++) = this->flags;

    *(buf++) = (uint8_t)(this->keepAlive >> 8);
    *(buf++) = (uint8_t)this->keepAlive;
    len = UTF8_encode(buf, this->clientID);
    if (len == -1) {
        return -1;
    }
    buf += len;

    if ((this->flags & WILL_FLAG) == WILL_FLAG) {
        len = UTF8_encode(buf, this->will->topic);
        if (len == -1) {
            return -1;
        }
        buf += len;
        len = UTF8_encode(buf, this->will->message);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    if ((this->flags & USERNAME_FLAG) == USERNAME_FLAG) {
        len = UTF8_encode(buf, this->user->name);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    if ((this->flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        len = UTF8_encode(buf, this->user->passwd);
        if (len == -1) {
            return -1;
        }
        buf += len;
    }
    return buf - wire;
}

int64_t ConnectMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    std::string name;

    int64_t len = UTF8_decode(buf, &name);
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
    len = UTF8_decode(buf, &(this->clientID));
    buf += len;

    if ((this->flags & WILL_FLAG) == WILL_FLAG) {
        std::string wTopic;
        len = UTF8_decode(buf, &wTopic);
        buf += len;
        std::string wMessage;
        len = UTF8_decode(buf, &wMessage);
        buf += len;
        bool wRetain = (this->flags & WILL_RETAIN_FLAG) == WILL_RETAIN_FLAG;
        uint8_t wQoS = (uint8_t)((this->flags & WILL_QOS3_FLAG) >> 3);
        this->will = new struct Will(wTopic, wMessage, wRetain, wQoS);
    }

    if ((this->flags & USERNAME_FLAG) == USERNAME_FLAG || (this->flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        std::string name(""), passwd("");
        if ((this->flags & USERNAME_FLAG) == USERNAME_FLAG) {
            len = UTF8_decode(buf, &name);
            buf += len;
        }
        if ((this->flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
            len = UTF8_decode(buf, &passwd);
            buf += len;
        }
        this->user = new struct User(name, passwd);
    }

    return buf - wire;
}

std::string ConnectMessage::flagString() {
    std::string out("");
    if ((this->flags & CLEANSESSION_FLAG) == CLEANSESSION_FLAG) {
        out += "\tCleanSession\n";
    }
    if ((this->flags & WILL_FLAG) == WILL_FLAG) {
        out += "\tWillFlag\n";
    }
    switch (this->flags & WILL_QOS3_FLAG) {
    case WILL_QOS0_FLAG:
        out += "\tWill_QoS0\n";
    case WILL_QOS1_FLAG:
        out += "\tWill_QoS1\n";
    case WILL_QOS2_FLAG:
        out += "\tWill_QoS2\n";
    case WILL_QOS3_FLAG:
        out += "\tWill_QoS3\n";

    }
    if ((this->flags & WILL_RETAIN_FLAG) == WILL_RETAIN_FLAG) {
        out += "\tWillRetain\n";
    }
    if ((this->flags & PASSWORD_FLAG) == PASSWORD_FLAG) {
        out += "\tPassword\n";
    }
    if ((this->flags & USERNAME_FLAG) == USERNAME_FLAG) {
        out += "\tUsername\n";
    }
    return out;
}

std::string ConnectMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "Protocol=" << protocol.name << ":" << protocol.level << ", Flags=\n" << flagString() << "\t, KeepAlive=" << keepAlive << ", ClientID=" << clientID << ", Will={" << this->will->topic << ":" << this->will->message << ", retain=" << this->will->retain << ", QoS=" << this->will->qos << "}, UserInfo={" << this->user->name << ":" << this->user->passwd << "}";
    return ss.str();
}


ConnackMessage::ConnackMessage(bool sp, ConnectReturnCode code) : sessionPresent(sp), returnCode(code), Message(new FixedHeader(CONNACK_MESSAGE_TYPE, false, 0, false, 2, 0)) {}

int64_t ConnackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    if (this->sessionPresent) {
        *(buf++) = 0x01;
    }
    *(buf++) = (uint8_t)this->returnCode;

    return buf - wire;
}

std::string ConnackMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "Session presentation=" << this->sessionPresent << ", Return code=" << this->returnCode;
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
        this->fh->length += 2;
    } else if (id != 0) {
        // warnning
    }
}

int64_t PublishMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    len = UTF8_encode(buf, this->topicName);
    if (this->fh->qos > 0) {
        *(buf++) = (uint8_t)(this->fh->packetID >> 8);
        *(buf++) = (uint8_t)this->fh->packetID;
    }
    memcpy(buf, this->payload.c_str(), this->payload.size());
    buf += this->payload.size();
    return buf - wire;
}

int64_t PublishMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;

    int64_t len = UTF8_decode(buf, &(this->topicName));
    buf += len;

    if (this->topicName.find('#') == std::string::npos || this->topicName.find('+') == std::string::npos) {
        err = WILDCARD_CHARACTERS_IN_PUBLISH;
        return -1;
    }


    if (this->fh->qos > 0) {
        this->fh->packetID = ((uint16_t)*(buf++) << 8);
        this->fh->packetID |= *(buf++);
    }
    int payloadLen = this->fh->length - (buf - wire - len);
    this->payload = std::string(buf, buf+payloadLen);

    return buf - wire;
}

std::string PublishMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID << ", Topic=" << this->topicName << ", Data=" << this->payload;
    return ss.str();
}

PubackMessage::PubackMessage(uint16_t id) : Message(new FixedHeader(PUBACK_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    return buf - wire;
}

int64_t PubackMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubackMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID;
    return ss.str();
}

PubrecMessage::PubrecMessage(uint16_t id) : Message(new FixedHeader(PUBREC_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubrecMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    return buf - wire;
}

int64_t PubrecMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubrecMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID;
    return ss.str();
}

PubrelMessage::PubrelMessage(uint16_t id) : Message(new FixedHeader(PUBREL_MESSAGE_TYPE, false, 1, false, 2, id)) {}

int64_t PubrelMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    return buf - wire;
}

int64_t PubrelMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubrelMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID;
    return ss.str();
}

PubcompMessage::PubcompMessage(uint16_t id) : Message(new FixedHeader(PUBCOMP_MESSAGE_TYPE, false, 0, false, 2, id)) {}

int64_t PubcompMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    return buf - wire;
}

int64_t PubcompMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);
    return buf - wire;
}

std::string PubcompMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID;
    return ss.str();
}

SubscribeMessage::SubscribeMessage(uint16_t id, std::vector<SubscribeTopic*> topics) : subTopics(topics), Message(new FixedHeader(SUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2+topics.size(), id)) {
    for (std::vector<SubscribeTopic*>::iterator it = this->subTopics.begin(); it != this->subTopics.end(); it++) {
        this->fh->length += (*it)->topic.size();
    }
}

SubscribeMessage::~SubscribeMessage() {
    for (std::vector<SubscribeTopic*>::iterator it = this->subTopics.begin(); it != this->subTopics.end(); it++) {
        delete (*it);
    }
}

int64_t SubscribeMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    for (int i = 0; i < this->subTopics.size(); i++) {
        len = UTF8_encode(buf, this->subTopics[i]->topic);
        if (len == -1) {
            return -1;
        }
        buf += len;
        *(buf++) = this->subTopics[i]->qos;
    }

    return buf - wire;
}

int64_t SubscribeMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    int64_t len = 0;
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);

    std::string topic;
    for (int i = 0; i < this->fh->length-2;) {
        topic = "";
        len = UTF8_decode(buf, &topic);

        buf += len;
        if (*buf == 3) {
            err = INVALID_QOS_3;
            return -1;
        } else if (*buf > 3) {
            err = MALFORMED_SUBSCRIBE_RESERVED_PART;
            return -1;
        }
        uint8_t qos = *(buf++) & 0x03;
        this->subTopics.push_back(new SubscribeTopic(topic, qos));
        i += len + 1;
    }

    return buf - wire;
}

std::string SubscribeMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID << "\n";
    for (int i = 0; i < this->subTopics.size(); i++) {
        ss << "\t" << i << ": Topic=" << this->subTopics[i]->topic << ", QoS=" << this->subTopics[i]->qos << "\n";
    }

    return ss.str();
}

SubackMessage::SubackMessage(uint16_t id, std::vector<SubackCode> codes) : returnCodes(codes), Message(new FixedHeader(SUBACK_MESSAGE_TYPE, false, 0, false, 2+codes.size(), id)) {}


int64_t SubackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    for (int i = 0; i < this->fh->length-2; i++) {
        *(buf++) = this->returnCodes[i];
    }
    return buf - wire;
}

int64_t SubackMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);
    for (int i = 0; i < this->fh->length-2; i++) {
        this->returnCodes.push_back((SubackCode)*(buf++));
    }

    return buf - wire;
}

std::string SubackMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID << "\n";
    for (int i = 0; i < this->returnCodes.size(); i++) {
        ss << "\t" << i << ": " << SubackCodeString[this->returnCodes[i]] << "\n";
    }

    return ss.str();
}

UnsubscribeMessage::UnsubscribeMessage(uint16_t id, std::vector<std::string> topics) : topics(topics), Message(new FixedHeader(UNSUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2 + 2*topics.size(), id)) {
    for (int i = 0; i < this->topics.size(); i++) {
        this->fh->length += this->topics[i].size();
    }
}

int64_t UnsubscribeMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    int payload_len = 2;
    for (int i = 0; payload_len < this->fh->length; i++) {
        len = UTF8_encode(buf, this->topics[i]);
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
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);
    std::string s;
    for (int i = 0; i < this->fh->length-2; i++) {
        s = "";
        len = UTF8_decode(buf, &s);
        this->topics.push_back(s);
        buf += len;
    }
    return buf - wire;
}

std::string UnsubscribeMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID << "\n";
    for (int i = 0; i < this->topics.size(); i++) {
        ss << "\t" << i << ": " << this->topics[i] << "\n";
    }

    return ss.str();
}

UnsubackMessage::UnsubackMessage(uint16_t id) : Message(new FixedHeader(UNSUBACK_MESSAGE_TYPE, false, 0, false, 2, id)) {};

int64_t UnsubackMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    *(buf++) = (uint8_t)(this->fh->packetID >> 8);
    *(buf++) = (uint8_t)this->fh->packetID;
    return buf - wire;
}

int64_t UnsubackMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    this->fh->packetID = ((uint16_t)*(buf++) << 8);
    this->fh->packetID |= *(buf++);
    return buf - wire;
}

std::string UnsubackMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString() << "packetID=" << this->fh->packetID << "\n";
    return ss.str();
}

PingreqMessage::PingreqMessage() : Message(new FixedHeader(PINGREQ_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t PingreqMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
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
    ss << this->fh->getString();
    return ss.str();
}


PingrespMessage::PingrespMessage() : Message(new FixedHeader(PINGRESP_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t PingrespMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
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
    ss << this->fh->getString();
    return ss.str();
}

DisconnectMessage::DisconnectMessage() : Message(new FixedHeader(DISCONNECT_MESSAGE_TYPE, false, 0, false, 0, 0)) {};

int64_t DisconnectMessage::getWire(uint8_t* wire) {
    uint8_t* buf = wire;
    int64_t len = this->fh->getWire(buf);
    if (len == -1) {
        return -1;
    }
    buf += len;
    return buf - wire;
}

std::string DisconnectMessage::getString() {
    std::stringstream ss;
    ss << this->fh->getString();
    return ss.str();
}

int64_t DisconnectMessage::parse(const uint8_t* wire, MQTT_ERROR& err) {
    return 0;
}
