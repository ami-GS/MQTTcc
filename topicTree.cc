#include "topicTree.h"
#include "frame.h"
#include <map>
#include <vector>

TopicNode::~TopicNode() {
    for (std::map<std::string, TopicNode*>::iterator itPair = nodes.begin(); itPair != nodes.end(); itPair++) {
        delete itPair->second;
    }
}

std::vector<TopicNode*> TopicNode::getNodesByNumberSign() {
    if (fullPath.find_last_of("/") != std::string::npos) {
        //fullPath = fullPath.substr(0, fullPath.find_last_of("/"));
    }
    std::vector<TopicNode*> resp;
    resp.push_back(this);
    if (nodes.size() > 0) {
        for (std::map<std::string, TopicNode*>::iterator itPair = nodes.begin(); itPair != nodes.end(); itPair++) {
            if (itPair->first[0] == '$') {
                continue;
            }
            std::vector<TopicNode*> respN = itPair->second->getNodesByNumberSign();
            resp.insert(resp.end(), respN.begin(), respN.end());            
        }
    }
    return resp;
}

std::vector<TopicNode*> TopicNode::getTopicNode(std::string topic) {
    std::vector<std::string> parts;
    size_t current = 0, found;
    std::string currentPath = "";
    while((found = topic.find_first_of("/", current)) != std::string::npos){
        parts.push_back(std::string(topic, current, found - current));
        current = found + 1;
    }
    parts.push_back(std::string(topic, current, topic.size() - current));

    TopicNode *nxt = this, *bef;
    std::vector<TopicNode*> resp;
    std::string part;
    for (int i = 0; i < parts.size(); i++) {
        bef = nxt;
        part = parts[i];
        if (part == "+") {
        for (std::map<std::string, TopicNode*>::iterator itPair = nodes.begin(); itPair != nodes.end(); itPair++) {
            if (itPair->first[0] == '$') {
                continue;
            }
            std::string filledStr = topic;
            filledStr.replace(topic.find_first_of("+"), 1, itPair->first);
            std::vector<TopicNode*> respN = getTopicNode(filledStr);
            resp.insert(resp.end(), respN.begin(), respN.end());
        }
        
        } else if (part == "#") {
            std::vector<TopicNode*> respN = getNodesByNumberSign();
            resp.insert(resp.end(), respN.begin(), respN.end());
        } else {
            currentPath += part;
            if (nodes.find(part) == nodes.end()) {
                //if there is no element
                return resp;
            }
            nxt = nodes[part];
            if (parts.size() - 1 == i) {
                resp.push_back(nxt);
            }
        }
    }
    return resp;
}

std::vector<SubackCode> TopicNode::applySubscriber(std::string clientID, std::string topic, uint8_t qos) {
    std::vector<TopicNode*> subNodes = getTopicNode(topic);
    std::vector<SubackCode> resp;
    if (subNodes.size() == 0) {
        return resp;
    }
    for (std::vector<TopicNode*>::iterator it = subNodes.begin(); it != subNodes.end(); it++) {
        // TODO: the return qos should be managed by broker
        (*it)->subscribers[clientID] = qos;
        resp.push_back((SubackCode)qos);
    }
    return resp;
}

int TopicNode::deleteSubscriber(std::string clientID, std::string topic) {
    std::vector<TopicNode*> subNodes = getTopicNode(topic);
    if (subNodes.size() == 0) {
        return -1;
    }
    for (std::vector<TopicNode*>::iterator it = subNodes.begin(); it != subNodes.end(); it++) {
        (*it)->subscribers.erase(clientID);
    }
    return 1;
}
