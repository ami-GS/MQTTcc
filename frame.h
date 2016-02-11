#ifndef MQTT_FRAME_H_
#define MQTT_FRAME_H_

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
    Will(std::string topic, std::string message, bool retain, uint8_t qos) : Topic(topic), Message(message), Retain(retain), QoS(qos) {};
    std::string Topic;
    std::string Message;
    bool Retain;
    uint8_t QoS;
};

struct User {
    User(std::string name, std::string pass) : Name(name), Passwd(pass) {};
    std::string Name;
    std::string Passwd;
};

const static struct MQTT_VERSION {
    std::string  name;
    uint8_t     level;
} MQTT_3_1_1 = {"MQTT", 4};

/*class Message {
public:
    virtual int64_t GetWire(uint8_t* wire);
    virtual std::string String();
    virtual int64_t
    }*/

class FixedHeader {
public:
    MessageType Type;
    bool Dup;
    bool Retain;
    uint8_t QoS;
    uint32_t Length;
    uint16_t PacketID;
    FixedHeader(MessageType type, bool dup, uint8_t qos, bool retain, uint32_t length, uint16_t id);
    FixedHeader(FixedHeader* fh) : Type(fh->Type), Dup(fh->Dup), QoS(fh->QoS), Retain(fh->Retain), Length(fh->Length), PacketID(fh->PacketID) {};
    ~FixedHeader() {};
    virtual int64_t GetWire(uint8_t* wire);
    virtual std::string String();
    virtual int64_t parse(uint8_t* wire);
    int64_t parseHeader(uint8_t* wire);
};

class ConnectMessage : public FixedHeader {
public:
    uint8_t Flags;
    uint16_t KeepAlive;
    std::string ClientID;
    bool CleanSession;
    struct Will* Will;
    struct User* User;
    struct MQTT_VERSION Protocol;

    ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, struct Will* will, struct User* user);
    ConnectMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~ConnectMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string FlagString();
    std::string String();
    int64_t parse(uint8_t* wire);
};


class ConnackMessage : public FixedHeader {
public:
    bool SessionPresent;
    ConnectReturnCode ReturnCode;
    ConnackMessage(bool sp, ConnectReturnCode code);
    ConnackMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~ConnackMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};


class PublishMessage : public FixedHeader {
public:
    std::string topicName;
    std::string payload;
    PublishMessage(bool dup, uint8_t qos, bool retain, uint16_t id, std::string topic, std::string payload);
    PublishMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~PublishMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class PubackMessage : public FixedHeader {
public:
    PubackMessage(uint16_t id);
    PubackMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~PubackMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};


class PubrecMessage : public FixedHeader {
public:
    PubrecMessage(uint16_t id);
    PubrecMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~PubrecMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class PubrelMessage : public FixedHeader {
public:
    PubrelMessage(uint16_t id);
    PubrelMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~PubrelMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class PubcompMessage : public FixedHeader {
public:
    PubcompMessage(uint16_t id);
    PubcompMessage();
    ~PubcompMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

struct SubscribeTopic {
    SubscribeTopic(std::string topic, uint8_t qos) : topic(topic), qos(qos) {};
    std::string topic;
    uint8_t qos;
};

class SubscribeMessage : public FixedHeader {
public:
    std::vector<SubscribeTopic*> subTopics;
    int topicNum;
    
    SubscribeMessage(uint16_t id, std::vector<SubscribeTopic*> topics, int tN);
    SubscribeMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~SubscribeMessage() {};
    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

enum SubackCode {
    ACK_MAX_QOS0 = 0,
    ACK_MAX_QOS1,
    ACK_MAX_QOS2,
    FAILURE,
};
static const std::string SubackCodeString[4] = {"ACK_MAX_QOS0", "ACK_MAX_QOS1", "ACK_MAX_QOS2", "FAILURE"};


class SubackMessage : public FixedHeader {
public:
    std::vector<SubackCode> returnCodes;
    int codeNum;
    
    SubackMessage(uint16_t id, std::vector<SubackCode> codes, int cN);
    SubackMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~SubackMessage() {};

    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class UnsubscribeMessage : public FixedHeader {
public:
    std::vector<std::string> topics;
    int topicNum;

    UnsubscribeMessage(uint16_t id, std::vector<std::string> topics, int tN);
    UnsubscribeMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~UnsubscribeMessage() {};

    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class UnsubackMessage : public FixedHeader {
public:
    UnsubackMessage(uint16_t id);
    UnsubackMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~UnsubackMessage() {};

    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class PingreqMessage : public FixedHeader {
public:
    PingreqMessage();
    PingreqMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~PingreqMessage() {};

    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class PingrespMessage : public FixedHeader {
public:
    PingrespMessage();
    PingrespMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~PingrespMessage() {};

    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};

class DisconnectMessage : public FixedHeader {
public:
    DisconnectMessage();
    DisconnectMessage(FixedHeader* fh) : FixedHeader(fh) {};
    ~DisconnectMessage() {};

    int64_t GetWire(uint8_t* wire);
    std::string String();
    int64_t parse(uint8_t* wire);
};










#endif // MQTT_FRAME_H
