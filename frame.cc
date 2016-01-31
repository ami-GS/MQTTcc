#include <stdint.h>
#include "frame.h"

FixedHeader::FixedHeader(MessageType type, bool dup, uint8_t qos, bool retain, uint32_t length, uint16_t id) :
Type(type), Dup(dup), QoS(qos), Retain(retain), Length(length), PacketID(id) {}

ConnectMessage::ConnectMessage(uint16_t keepAlive, std::string id, bool cleanSession, struct Will* will, struct User* user) :
    KeepAlive(keepAlive), ClientID(id), CleanSession(cleanSession), Will(will), User(user), Flags(0), FixedHeader(CONNECT_MESSAGE_TYPE, false, 0, false, 0, 0) {
    uint32_t length = 6 + MQTT_3_1_1_NAME.size() + 2 + id.size();
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

ConnackMessage::ConnackMessage(bool sp, ConnectReturnCode code) : SessionPresent(sp), ReturnCode(code), FixedHeader(CONNACK_MESSAGE_TYPE, false, 0, false, 2, 0) {}

PublishMessage::PublishMessage(bool dup, uint8_t qos, bool retain, uint16_t id, std::string topic, std::string payload) : topicName(topic), payload(payload), FixedHeader(PUBLISH_MESSAGE_TYPE, dup, qos, retain, topic.size()+payload.size()+2, id) {
    if (qos > 0) {
        Length += 2;
    } else if (id != 0) {
        // warnning
    }
}

PubackMessage::PubackMessage(uint16_t id) : FixedHeader(PUBACK_MESSAGE_TYPE, false, 0, false, 2, id) {}
PubrecMessage::PubrecMessage(uint16_t id) : FixedHeader(PUBREC_MESSAGE_TYPE, false, 0, false, 2, id) {}
PubrelMessage::PubrelMessage(uint16_t id) : FixedHeader(PUBREL_MESSAGE_TYPE, false, 1, false, 2, id) {}
PubcompMessage::PubcompMessage(uint16_t id) : FixedHeader(PUBCOMP_MESSAGE_TYPE, false, 0, false, 2, id) {}

SubscribeMessage::SubscribeMessage(uint16_t id, SubscribeTopic** topics, int topicNum) : subTopics(topics), FixedHeader(SUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2+topicNum, id) {
    for (int i = 0; i < topicNum; i++) {
        Length += topics[i]->topic.size();
    }
}

SubackMessage::SubackMessage(uint16_t id, SubackCode* codes, int codeNum) : returnCodes(codes), FixedHeader(SUBACK_MESSAGE_TYPE, false, 0, false, 2+codeNum, id) {}


UnsubscribeMessage::UnsubscribeMessage(uint16_t id, std::string* topics, int topicNum) : topics(topics), FixedHeader(UNSUBSCRIBE_MESSAGE_TYPE, false, 1, false, 2 + 2*topicNum, id) {
    for (int i = 0; i < topicNum; i++) {
        Length += (*topics).size();
    }
}

UnsubackMessage::UnsubackMessage(uint16_t id) : FixedHeader(UNSUBACK_MESSAGE_TYPE, false, 0, false, 2, id) {};
PingreqMessage::PingreqMessage() : FixedHeader(PINGREQ_MESSAGE_TYPE, false, 0, false, 0, 0) {};
PingrespMessage::PingrespMessage() : FixedHeader(PINGRESP_MESSAGE_TYPE, false, 0, false, 0, 0) {};
DisconnectMessage::DisconnectMessage() : FixedHeader(DISCONNECT_MESSAGE_TYPE, false, 0, false, 0, 0) {};
