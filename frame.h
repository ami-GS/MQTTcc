#ifndef MQTT_FRAME_H_
#define MQTT_FRAME_H_

#include <stdint.h>
#include <string>

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

//typedef uint8_t ConnectFlag;
enum ConnectFlag {
    RESERVED_FLAG = 1,
    CLEANSESSION_FLAG,
    WILL_FLAG,
    WILL_QOS0_FLAG,
    WILL_QOS1_FLAG,
    WILL_QOS2_FLAG,
    WILL_QOS3_FLAG,
    WILL_RETAIN_FLAG,
    PASSWORD_FLAG,
    USERNAME_FLAG,
};

enum ConnectReturnCode {
    CONNECT_ACCEPTED = 0,
    CONNECT_UNNACCEPTABLE_PROTOCOL_VERSION,
    CONNECT_IDENTIFIER_REJECTED,
    CONNECT_SERVER_UNAVAILABLE,
    CONNECT_BAD_USERNAME_OR_PASSWORD,
    CONNECT_NOT_AUTHORIZED,
};

struct Will {
    std::string Topic;
    std::string Message;
    bool Retain;
    uint8_t QoS;
};

struct User {
    std::string Name;
    std::string Passwd;
};

const static struct {
    std::string  name;
    uint8_t     level;
} MQTT_3_1_1 = {"MQTT", 4};

class FixedHeader {
public:
    MessageType Type;
    bool Dup;
    bool Retain;
    uint8_t QoS;
    uint32_t Length;
    uint16_t PacketID;
    FixedHeader(MessageType type, bool dup, uint8_t qos, bool retain, uint32_t length, uint16_t id); 
    ~FixedHeader() {};
    virtual int64_t GetWire(uint8_t* wire);
    virtual std::string String();
};

class ConnectMessage : public FixedHeader {
    uint8_t Flags;
    uint16_t KeepAlive;
    std::string ClientID;
    bool CleanSession;
    struct Will* Will;
    struct User* User;

    ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, struct Will* will, struct User* user);
    ~ConnectMessage() {};
    int64_t GetWire(uint8_t* wire);
};


class ConnackMessage : public FixedHeader {
    bool SessionPresent;
    ConnectReturnCode ReturnCode;
    ConnackMessage(bool sp, ConnectReturnCode code);
    ~ConnackMessage() {};
    int64_t GetWire(uint8_t* wire);
};


class PublishMessage : public FixedHeader {
    std::string topicName;
    std::string payload;
    PublishMessage(bool dup, uint8_t qos, bool retain, uint16_t id, std::string topic, std::string payload);
    ~PublishMessage() {};
    int64_t GetWire(uint8_t* wire);
};

class PubackMessage : public FixedHeader {
    PubackMessage(uint16_t id);
    ~PubackMessage() {};
    int64_t GetWire(uint8_t* wire);
};


class PubrecMessage : public FixedHeader {
    PubrecMessage(uint16_t id);
    ~PubrecMessage() {};
    int64_t GetWire(uint8_t* wire);
};

class PubrelMessage : public FixedHeader {
    PubrelMessage(uint16_t id);
    ~PubrelMessage() {};
    int64_t GetWire(uint8_t* wire);
};

class PubcompMessage : public FixedHeader {
    PubcompMessage(uint16_t id);
    ~PubcompMessage() {};
    int64_t GetWire(uint8_t* wire);
};

struct SubscribeTopic {
    std::string topic;
    uint8_t qos;
};

class SubscribeMessage : public FixedHeader {
    SubscribeTopic** subTopics;
    
    SubscribeMessage(uint16_t id, SubscribeTopic** topics, int topicNum);
    ~SubscribeMessage() {};
    int64_t GetWire(uint8_t* wire);
};

enum SubackCode {
    ACK_MAX_QOS0 = 0,
    ACK_MAX_QOS1,
    ACK_MAX_QOS2,
    FAILURE,
};

class SubackMessage : public FixedHeader {
    SubackCode* returnCodes;

    SubackMessage(uint16_t id, SubackCode* codes, int codeNum);
    ~SubackMessage() {};

    int64_t GetWire(uint8_t* wire);
};

class UnsubscribeMessage : public FixedHeader {
    std::string* topics;

    UnsubscribeMessage(uint16_t id, std::string* topics, int topicNum);
    ~UnsubscribeMessage() {};

    int64_t GetWire(uint8_t* wire);
};

class UnsubackMessage : public FixedHeader {
    UnsubackMessage(uint16_t id);
    ~UnsubackMessage() {};

    int64_t GetWire(uint8_t* wire);
};

class PingreqMessage : public FixedHeader {
    PingreqMessage();
    ~PingreqMessage() {};

    int64_t GetWire(uint8_t* wire);
};

class PingrespMessage : public FixedHeader {
    PingrespMessage();
    ~PingrespMessage() {};

    int64_t GetWire(uint8_t* wire);
};

class DisconnectMessage : public FixedHeader {
    DisconnectMessage();
    ~DisconnectMessage() {};

    int64_t GetWire(uint8_t* wire);
};










#endif // MQTT_FRAME_H
