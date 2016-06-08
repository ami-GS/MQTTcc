#ifndef MQTT_FRAME_H_
#define MQTT_FRAME_H_

#include "mqttError.h"
#include <stdint.h>
#include <string>
#include <vector>

//typedef MessageType uint8_t;

enum MessageType {
    RESERVED_0 = 0,
    CONNECT_MESSAGE_TYPE,
    CONNACK_MESSAGE_TYPE,
    PUBLISH_MESSAGE_TYPE,
    PUBACK_MESSAGE_TYPE,
    PUBREC_MESSAGE_TYPE,
    PUBREL_MESSAGE_TYPE,
    PUBCOMP_MESSAGE_TYPE,
    SUBSCRIBE_MESSAGE_TYPE,
    SUBACK_MESSAGE_TYPE,
    UNSUBSCRIBE_MESSAGE_TYPE,
    UNSUBACK_MESSAGE_TYPE,
    PINGREQ_MESSAGE_TYPE,
    PINGRESP_MESSAGE_TYPE,
    DISCONNECT_MESSAGE_TYPE,
    RESERVED_15,
};

static const std::string TypeString[] = {
    "RESERVED_0",
    "CONNECT",
    "CONNACK",
    "PUBLISH"
    "PUBACK",
    "PUBREC",
    "PUBREL",
    "PUBCOMP",
    "SUBSCRIBE",
    "SUBACK",
    "UNSUBSCRIBE",
    "UNSUBACK",
    "PINGREQ",
    "PINGRESP",
    "DISCONNECT",
    "RESERVED_15",
};

typedef uint8_t ConnectFlag;
const static ConnectFlag RESERVED_FLAG     = 0x01;
const static ConnectFlag CLEANSESSION_FLAG = 0x02;
const static ConnectFlag WILL_FLAG         = 0x04;
const static ConnectFlag WILL_QOS0_FLAG    = 0x00;
const static ConnectFlag WILL_QOS1_FLAG    = 0x08;
const static ConnectFlag WILL_QOS2_FLAG    = 0x10;
const static ConnectFlag WILL_QOS3_FLAG    = 0x18;
const static ConnectFlag WILL_RETAIN_FLAG  = 0x20;
const static ConnectFlag PASSWORD_FLAG     = 0x40;
const static ConnectFlag USERNAME_FLAG     = 0x80;

enum ConnectReturnCode {
    CONNECT_ACCEPTED = 0,
    CONNECT_UNNACCEPTABLE_PROTOCOL_VERSION,
    CONNECT_IDENTIFIER_REJECTED,
    CONNECT_SERVER_UNAVAILABLE,
    CONNECT_BAD_USERNAME_OR_PASSWORD,
    CONNECT_NOT_AUTHORIZED,
};

struct Will {
    Will(std::string topic, std::string message, bool retain, uint8_t qos) : topic(topic), message(message), retain(retain), qos(qos) {};
    std::string topic;
    std::string message;
    bool retain;
    uint8_t qos;
};

struct User {
    User(std::string name, std::string pass) : name(name), passwd(pass) {};
    std::string name;
    std::string passwd;
};

const static struct MQTT_VERSION {
    std::string  name;
    uint8_t     level;
} MQTT_3_1_1 = {"MQTT", 4};

class FixedHeader {
public:
    MessageType type;
    bool dup;
    bool retain;
    uint8_t qos;
    uint32_t length;
    uint16_t packetID;
    FixedHeader(MessageType type, bool dup, uint8_t qos, bool retain, uint32_t length, uint16_t id);
    FixedHeader() {};
    ~FixedHeader() {};
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parseHeader(const uint8_t* wire, MQTT_ERROR& err);
};

class Message {
public:
    FixedHeader* fh;
    Message(FixedHeader* fh);
    virtual ~Message();
    virtual int64_t getWire(uint8_t* wire) = 0;
    virtual std::string getString() = 0;
    virtual int64_t parse(const uint8_t* wire, MQTT_ERROR& err) = 0;
};

class ConnectMessage : public Message {
public:
    uint8_t flags;
    uint16_t keepAlive;
    std::string clientID;
    bool cleanSession;
    const Will* will;
    const User* user;
    struct MQTT_VERSION protocol;

    ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, const struct Will* will, const struct User* user);
    ConnectMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~ConnectMessage();
    int64_t getWire(uint8_t* wire);
    std::string flagString();
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};


class ConnackMessage : public Message {
public:
    bool sessionPresent;
    ConnectReturnCode returnCode;
    ConnackMessage(bool sp, ConnectReturnCode code);
    ConnackMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~ConnackMessage() {};
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};


class PublishMessage : public Message {
public:
    std::string topicName;
    std::string payload;
    PublishMessage(bool dup, uint8_t qos, bool retain, uint16_t id, std::string topic, std::string payload);
    PublishMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~PublishMessage() {};
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class PubackMessage : public Message {
public:
    PubackMessage(uint16_t id);
    PubackMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~PubackMessage() {};
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};


class PubrecMessage : public Message {
public:
    PubrecMessage(uint16_t id);
    PubrecMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~PubrecMessage() {};
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class PubrelMessage : public Message {
public:
    PubrelMessage(uint16_t id);
    PubrelMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~PubrelMessage() {};
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class PubcompMessage : public Message {
public:
    PubcompMessage(uint16_t id);
    PubcompMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~PubcompMessage() {};
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

struct SubscribeTopic {
    SubscribeTopic(std::string topic, uint8_t qos) : topic(topic), qos(qos) {};
    std::string topic;
    uint8_t qos;
};

class SubscribeMessage : public Message {
public:
    std::vector<SubscribeTopic*> subTopics;
    
    SubscribeMessage(uint16_t id, std::vector<SubscribeTopic*> topics);
    SubscribeMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~SubscribeMessage();
    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

enum SubackCode {
    ACK_MAX_QOS0 = 0,
    ACK_MAX_QOS1,
    ACK_MAX_QOS2,
    FAILURE,
};
static const std::string SubackCodeString[4] = {"ACK_MAX_QOS0", "ACK_MAX_QOS1", "ACK_MAX_QOS2", "FAILURE"};


class SubackMessage : public Message {
public:
    std::vector<SubackCode> returnCodes;
    
    SubackMessage(uint16_t id, std::vector<SubackCode> codes);
    SubackMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~SubackMessage() {};

    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class UnsubscribeMessage : public Message {
public:
    std::vector<std::string> topics;

    UnsubscribeMessage(uint16_t id, std::vector<std::string> topics);
    UnsubscribeMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~UnsubscribeMessage() {};

    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class UnsubackMessage : public Message {
public:
    UnsubackMessage(uint16_t id);
    UnsubackMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~UnsubackMessage() {};

    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class PingreqMessage : public Message {
public:
    PingreqMessage();
    PingreqMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~PingreqMessage() {};

    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class PingrespMessage : public Message {
public:
    PingrespMessage();
    PingrespMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~PingrespMessage() {};

    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

class DisconnectMessage : public Message {
public:
    DisconnectMessage();
    DisconnectMessage(FixedHeader* fh, const uint8_t* wire, MQTT_ERROR& err) : Message(fh) {this->parse(wire, err);};
    ~DisconnectMessage() {};

    int64_t getWire(uint8_t* wire);
    std::string getString();
    int64_t parse(const uint8_t* wire, MQTT_ERROR& err);
};

#endif // MQTT_FRAME_H
