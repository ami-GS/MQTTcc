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
    std::vector<TopicNode*> getTopicNode(std::string topic, bool addNewNode);
    std::vector<SubackCode> applySubscriber(std::string clientID, std::string topic, uint8_t qos);
    int deleteSubscriber(std::string clientID, std::string topic);
    int applyRetain(std::string topic, uint8_t qos, std::string retain);

};


#endif // MQTT_TOPICTREE_H_
