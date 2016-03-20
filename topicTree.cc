#include "topicTree.h"
#include "frame.h"
#include "util.h"
#include <map>
#include <vector>

TopicNode::TopicNode(std::string topic) : nodes(), fullPath(topic), retainMessage(""), retainQoS(0), subscribers() {}

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

std::vector<TopicNode*> TopicNode::getTopicNode(const std::string topic, bool addNewNode, MQTT_ERROR& err) {
    std::string currentPath = "";
    std::vector<std::string> parts;
    split(topic, "/", &parts);
    TopicNode *nxt = this, *bef;
    std::vector<TopicNode*> resp;
    std::string part;
    for (int i = 0; i < parts.size(); i++) {
        bef = nxt;
        part = parts[i];
        if (part == "+") {
            for (std::map<std::string, TopicNode*>::iterator itPair = bef->nodes.begin(); itPair != bef->nodes.end(); itPair++) {
                if (itPair->first[0] == '$') {
                    continue;
                }
                std::string filledStr = topic;
                filledStr.replace(topic.find_first_of("+"), 1, itPair->first);
                std::vector<TopicNode*> respN = getTopicNode(filledStr, addNewNode, err);
                if (err != NO_ERROR) {
                    return resp;
                }
                resp.insert(resp.end(), respN.begin(), respN.end());
            }
        } else if (part == "#") {
            if (i != parts.size() - 1) {
                err = MULTI_LEVEL_WILDCARD_MUST_BE_ON_TAIL;
                return resp;
            }
            std::vector<TopicNode*> respN = getNodesByNumberSign();
            resp.insert(resp.end(), respN.begin(), respN.end());
        } else {
            //if (/*has suffix of # or +*/) {
            //    err = WILDCARD_MUST_NOT_BE_ADJACENT_TO_NAME;
            //}
            currentPath += part;
            if (part.size()-1 != i) {
                currentPath += "/";
            }
            if (bef->nodes.find(part) == bef->nodes.end() && addNewNode) {
                bef->nodes[part] = new TopicNode(currentPath);
            } else if (bef->nodes.find(part) == bef->nodes.end()) {
                continue;
            }
            nxt = bef->nodes[part];
            if (parts.size() - 1 == i) {
                resp.push_back(nxt);
            }
        }
    }
    return resp;
}

std::vector<SubackCode> TopicNode::applySubscriber(const std::string clientID, const std::string topic, uint8_t qos, MQTT_ERROR& err) {
    std::vector<SubackCode> resp;
    std::vector<TopicNode*> subNodes = getTopicNode(topic, true, err);
    if (err != NO_ERROR) {
        return resp;
    }
    for (std::vector<TopicNode*>::iterator it = subNodes.begin(); it != subNodes.end(); it++) {
        // TODO: the return qos should be managed by broker
        (*it)->subscribers[clientID] = qos;
        resp.push_back((SubackCode)qos);
    }
    return resp;
}

int TopicNode::deleteSubscriber(const std::string clientID, const std::string topic, MQTT_ERROR& err) {
    std::vector<TopicNode*> subNodes = getTopicNode(topic, false, err);
    if (err != NO_ERROR) {
        return -1;
    }
    for (std::vector<TopicNode*>::iterator it = subNodes.begin(); it != subNodes.end(); it++) {
        (*it)->subscribers.erase(clientID);
    }
    return 1;
}

int TopicNode::applyRetain(const std::string topic, uint8_t qos, const std::string retain, MQTT_ERROR& err) {
    std::vector<TopicNode*> retainNodes = getTopicNode(topic, true, err);
    if (err != NO_ERROR) {
        return -1;
    }
    for (std::vector<TopicNode*>::iterator it = retainNodes.begin(); it != retainNodes.end(); it++) {
        (*it)->retainMessage = retain;
        (*it)->retainQoS = qos;
    }
    return 1;
}

std::string TopicNode::dumpTree() {
    if (nodes.size() == 0) {
        return fullPath + "\n";
    }
    std::string str = "";
    for (std::map<std::string, TopicNode*>::iterator itPair = nodes.begin(); itPair != nodes.end(); itPair++) {
        str += itPair->second->dumpTree();
    }
    return str;
}
