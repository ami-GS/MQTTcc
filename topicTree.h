#ifndef MQTT_TOPICTREE_H_
#define MQTT_TOPICTREE_H_

#include "frame.h"
#include <map>
#include <vector>
#include <string>

class TopicNode {
    std::map<std::string, TopicNode*> nodes;
    std::string fullPath;
    std::map<std::string, uint8_t> subscribers;
    std::string allTopicName[65536]; //only for root, for broker?
    std::vector<TopicNode*> getNodesByNumberSign();
public:
    std::string retainMessage;
    uint8_t retainQoS;
    TopicNode(std::string topic);
    ~TopicNode();
    std::vector<TopicNode*> getTopicNode(const std::string topic, bool addNewNode);
    std::vector<SubackCode> applySubscriber(const std::string clientID, const std::string topic, uint8_t qos);
    int deleteSubscriber(const std::string clientID, const std::string topic);
    int applyRetain(const std::string topic, uint8_t qos, const std::string retain);
    std::string dumpTree();
};


#endif // MQTT_TOPICTREE_H_
