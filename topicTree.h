#ifndef MQTT_TOPICTREE_H_
#define MQTT_TOPICTREE_H_

#include "frame.h"
#include "mqttError.h"
#include <map>
#include <vector>
#include <string>

class TopicNode {
    std::map<std::string, TopicNode*> nodes;
    std::string name;
    std::string allTopicName[65536]; //only for root, for broker?
    MQTT_ERROR getNodesByNumberSign(std::vector<TopicNode*>* resp);
public:
    std::map<std::string, uint8_t> subscribers;
    std::string fullPath;
    std::string retainMessage;
    uint8_t retainQoS;
    TopicNode(std::string part, std::string fPath);
    ~TopicNode();
    MQTT_ERROR getTopicNode(const std::string topic, bool addNewNode, std::vector<TopicNode*>* resp);
    MQTT_ERROR applySubscriber(const std::string clientID, const std::string topic, uint8_t qos, std::vector<SubackCode>* resp);
    MQTT_ERROR deleteSubscriber(const std::string clientID, const std::string topic);
    MQTT_ERROR applyRetain(const std::string topic, uint8_t qos, const std::string retain);
    std::vector<std::string> dumpTree();
};


#endif // MQTT_TOPICTREE_H_
