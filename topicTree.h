#ifndef MQTT_TOPICTREE_H_
#define MQTT_TOPICTREE_H_

#include "frame.h"
#include <map>
#include <vector>
#include <string>

class TopicNode {
    std::map<std::string, TopicNode*> nodes;
    std::string fullPath;
    std::string retainMessage;
    uint8_t retainQoS;
    std::map<std::string, uint8_t> subscribers;
    std::string allTopicName[65536]; //only for root, for broker?
    std::vector<TopicNode*> getNodesByNumberSign();
public:
    TopicNode(std::string topic);
    ~TopicNode();
    std::vector<TopicNode*> getTopicNode(std::string topic);
    std::vector<SubackCode> applySubscriber(std::string clientID, std::string topic, uint8_t qos);
    int deleteSubscriber(std::string clientID, std::string topic);

};


#endif // MQTT_TOPICTREE_H_
