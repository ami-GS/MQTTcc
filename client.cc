#include "client.h"
#include "util.h"

Client::Client(const std::string id, const User* user, uint16_t keepAlive, const Will* will) : ID(id), user(user), keepAlive(keepAlive), will(will), isConnecting(false) {}

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
  return 1; // TODO:apply random
}

int Client::ackMessage(uint16_t pID) {
    if (packetIDMap.find(pID) == packetIDMap.end()) {
        return -1; // packet id does not exist
    }
    packetIDMap.erase(pID);
    return 1;
}

int64_t Client::connect(const std::string addr, int port, bool cleanSession) {
    if (ID.size() == 0 && !cleanSession) {
        return -1; // clean session must be ture
    }

    ct = new Transport(addr, port);
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
