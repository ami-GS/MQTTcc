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
    RESERVED_15
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

static const std::string MQTT_3_1_1_NAME = "MQTT";
static const uint8_t MQTT_3_1_1_LEVEL = 4;


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
    int64_t GetWire(uint8_t* buf);
    std::string String();
};

class ConnectMessage : public FixedHeader {
    uint8_t Flags;
    uint16_t KeepAlive;
    std::string ClientID;
    bool CleanSession;
    struct Will* Will;
    struct User* User;

    ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, struct Will* will, struct User* user);
};


#endif // MQTT_FRAME_H
