#include "client.h"
#include "util.h"
#include <random>
#include <chrono>

Client::Client(const std::string id, const User* user, uint16_t keepAlive, const Will* will) : ID(id), user(user), cleanSession(false), keepAlive(keepAlive), will(will), isConnecting(false), randPacketID(1, 65535) {
  std::random_device rnd;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  mt(); // TODO: apply seed
}

Client::~Client() {
    delete user;
    delete will;
}

int64_t Client::sendMessage(Message* m) {
    if (!isConnecting) {
        return -1; // not connecting
    }
    uint16_t packetID = m->fh->PacketID;
    if (packetIDMap.find(packetID) != packetIDMap.end()) {
        return -1; // packet ID has already used
    }
    int64_t len = ct->sendMessage(m);
    if (len != -1) {
        if (m->fh->Type == PUBLISH_MESSAGE_TYPE) {
            if (packetID > 0) {
                packetIDMap[packetID] = m;
            }
        } else if (m->fh->Type == PUBREC_MESSAGE_TYPE || m->fh->Type == SUBSCRIBE_MESSAGE_TYPE || m->fh->Type == UNSUBSCRIBE_MESSAGE_TYPE || m->fh->Type == PUBREL_MESSAGE_TYPE) {
            if (packetID == 0) {
                return -1; // packet id should not be zero
            }
            packetIDMap[packetID] = m;
        }
    }
    return len;
}

int32_t Client::getUsablePacketID() {
  bool exists = true;
  uint16_t id = 0;
  for (int trial = 0; exists; trial++) {
    if (trial == 5) {
      return -1; // fail to set packet id
    }
    id = randPacketID(mt);
    exists = !(packetIDMap.find(id) == packetIDMap.end());
  }
  return id;
}

int Client::ackMessage(uint16_t pID) {
    if (packetIDMap.find(pID) == packetIDMap.end()) {
        return -1; // packet id does not exist
    }
    packetIDMap.erase(pID);
    return 1;
}

int64_t Client::connect(const std::string addr, int port, bool cs) {
    if (ID.size() == 0 && !cleanSession) {
        return -1; // clean session must be ture
    }

    ct = new Transport(addr, port);
    cleanSession = cs;
    return ct->sendMessage(new ConnectMessage(keepAlive, ID, cleanSession, will, user));
}

int64_t Client::publish(const std::string topic, const std::string data, uint8_t qos, bool retain) {
  if (qos >= 3) {
    return -1; // invalid qos 3
  }
  //if ()

  int32_t id = 0;
  if (qos > 0) {
    id = getUsablePacketID();
    if (id == -1) {
      return -1;
    }
  }
  return sendMessage(new PublishMessage(false, qos, retain, id, topic, data));
}

int64_t Client::subscribe(std::vector<SubscribeTopic*> topics) {
  int32_t id = getUsablePacketID();
  if (id == -1) {
    return -1;
  }
  for (int i = 0; i < topics.size(); i++) {
    std::vector<std::string> parts;
    split(topics[i]->topic, "/", &parts);

    for (int j = 0; i < parts.size(); i++) {
      if (parts[j][0] == '#' && j != parts.size() - 1) {
	return -1; // multi level wildcard must be on tail
      } else if (false) {
      } // has suffix of '#' and '+'
    }
  }
  return sendMessage(new SubscribeMessage(id, topics, topics.size()));
}

int64_t Client::unsubscribe(std::vector<std::string> topics) {
  for (int i = 0; i < topics.size(); i++) {
    std::vector<std::string> parts;
    split(topics[i], "/", &parts);
    for (int j = 0; j < parts.size(); j++) {
      if (parts[j][0] == '#' && j != parts.size() - 1) {
	return -1; // multi level wildcard must be on tail
      } else if (false) {
      } // has suffix of '#' and '+'
    }
  }
  int32_t id = getUsablePacketID();
  if (id == -1) {
    return -1;
  }
  return sendMessage(new UnsubscribeMessage(id, topics, topics.size()));
}

int Client::redelivery() {
  int64_t len;
  if (!cleanSession && packetIDMap.size() > 0) {
    for (std::map<uint16_t, Message*>::iterator itPair = packetIDMap.begin(); itPair != packetIDMap.end(); itPair++) {
      len = sendMessage(itPair->second);
      if (len != -1) {
	return -1;
      }
    }
  }
  return 1;
}

void Client::setPreviousSession(Client* ps) {
  packetIDMap = ps->packetIDMap;
  cleanSession = ps->cleanSession;
  will = ps->will;
  user = ps->user;
  keepAlive = ps->keepAlive;
}

int Client::recvConnectMessage(ConnectMessage* m) {return -1;}
int Client::recvConnackMessage(ConnackMessage* m) {return 0;}
int Client::recvPublishMessage(PublishMessage* m) {return 0;}
int Client::recvPubackMessage(PubackMessage* m) {return 0;}
int Client::recvPubrecMessage(PubrecMessage* m) {return 0;}
int Client::recvPubrelMessage(PubrelMessage* m) {return 0;}
int Client::recvPubcompMessage(PubcompMessage* m) {return 0;}
int Client::recvSubscribeMessage(SubscribeMessage* m) {return -1;}
int Client::recvSubackMessage(SubackMessage* m) {return 0;}
int Client::recvUnsubscribeMessage(UnsubscribeMessage* m) {return -1;}
int Client::recvUnsubackMessage(UnsubackMessage* m) {return 0;}
int Client::recvPingreqMessage(PingreqMessage* m) {return -1;}
int Client::recvPingrespMessage(PingrespMessage* m) {return 0;}
int Client::recvDisconnectMessage(DisconnectMessage* m) {return -1;}

