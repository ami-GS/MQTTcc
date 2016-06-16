#include "topicTree.h"
#include "frame.h"
#include "util.h"
#include <map>
#include <vector>

TopicNode::TopicNode(std::string part, std::string fPath) : name(part), nodes(), fullPath(fPath), retainMessage(""), retainQoS(0), subscribers() {}

TopicNode::~TopicNode() {
    for (std::map<std::string, TopicNode*>::iterator itPair = nodes.begin(); itPair != nodes.end(); itPair++) {
        delete itPair->second;
    }
}

MQTT_ERROR TopicNode::getNodesByNumberSign(std::vector<TopicNode*>* resp) {
    if (fullPath.find_last_of("/") != std::string::npos) {
        fullPath = fullPath.substr(0, fullPath.size()-1);
    }
    MQTT_ERROR err = NO_ERROR;
    resp->push_back(this);
    if (nodes.size() > 0) {
        for (std::map<std::string, TopicNode*>::iterator itPair = nodes.begin(); itPair != nodes.end(); itPair++) {
            if (itPair->first[0] == '$') {
                continue;
            }
            err = itPair->second->getNodesByNumberSign(resp);
            if (err != NO_ERROR) {
                return err;
            }
        }
    }
    return err;
}

MQTT_ERROR TopicNode::getTopicNode(const std::string topic, bool addNewNode, std::vector<TopicNode*>* resp) {
    std::string currentPath = "";
    std::vector<std::string> parts;
    split(topic, "/", &parts);
    TopicNode *nxt = this, *bef;
    MQTT_ERROR err = NO_ERROR;
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
                err = getTopicNode(filledStr, addNewNode, resp);
                if (err != NO_ERROR) {
                    return err;
                }
            }
        } else if (part == "#") {
            if (i != parts.size() - 1) {
                return MULTI_LEVEL_WILDCARD_MUST_BE_ON_TAIL;
            }
            err = getNodesByNumberSign(resp);
        } else {
                if (part.find_last_of("#") != std::string::npos ||
                    part.find_last_of("+") != std::string::npos) {
                    return WILDCARD_MUST_NOT_BE_ADJACENT_TO_NAME;
                }
            currentPath += part;
            if (part.size()-1 != i) {
                currentPath += "/";
            }
            if (bef->nodes.find(part) == bef->nodes.end() && addNewNode) {
                bef->nodes[part] = new TopicNode(part, currentPath);
            } else if (bef->nodes.find(part) == bef->nodes.end()) {
                continue;
            }
            nxt = bef->nodes[part];
            if (parts.size() - 1 == i) {
                resp->push_back(nxt);
            }
        }
    }
    return err;
}

MQTT_ERROR TopicNode::applySubscriber(const std::string clientID, const std::string topic, uint8_t qos, std::vector<SubackCode>* resp) {
    std::vector<TopicNode*> subNodes;
    MQTT_ERROR err = getTopicNode(topic, true, &subNodes);
    if (err != NO_ERROR) {
        return err;
    }
    for (std::vector<TopicNode*>::iterator it = subNodes.begin(); it != subNodes.end(); it++) {
        // TODO: the return qos should be managed by broker
        (*it)->subscribers[clientID] = qos;
        resp->push_back((SubackCode)qos);
    }
    return err;
}

MQTT_ERROR TopicNode::deleteSubscriber(const std::string clientID, const std::string topic) {
    std::vector<TopicNode*> subNodes;
    MQTT_ERROR err= getTopicNode(topic, false, &subNodes);
    if (err != NO_ERROR) {
        return err;
    }
    for (std::vector<TopicNode*>::iterator it = subNodes.begin(); it != subNodes.end(); it++) {
        (*it)->subscribers.erase(clientID);
    }
    return err;
}

MQTT_ERROR TopicNode::applyRetain(const std::string topic, uint8_t qos, const std::string retain) {
    std::vector<TopicNode*> retainNodes;
    MQTT_ERROR err = getTopicNode(topic, true, &retainNodes);
    if (err != NO_ERROR) {
        return err;
    }
    for (std::vector<TopicNode*>::iterator it = retainNodes.begin(); it != retainNodes.end(); it++) {
        (*it)->retainMessage = retain;
        (*it)->retainQoS = qos;
    }
    return err;
}

std::vector<std::string> TopicNode::dumpTree() {
    std::vector<std::string> strs;
    if (nodes.size() == 0) {
        strs.push_back(this->name);
        return strs;
    }
    for (std::map<std::string, TopicNode*>::iterator itPair = nodes.begin(); itPair != nodes.end(); itPair++) {
        std::vector<std::string> deepStrs = itPair->second->dumpTree();
        std::string currentPath = "";
        if (this->name.size() > 0) {
            currentPath = this->name + "/";
       }
        for (std::vector<std::string>::iterator it = deepStrs.begin(); it != deepStrs.end(); it++) {
            strs.push_back(currentPath + *it);
        }
    }
    return strs;
}
