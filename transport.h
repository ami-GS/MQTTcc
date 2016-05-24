#ifndef MQTT_TRANSPORT_H_
#define MQTT_TRANSPORT_H_

#include <stdint.h>
#include <netinet/in.h>
#include "frame.h"
#include "mqttError.h"

class Transport {
    int sock;
    struct sockaddr_in* target;
    uint8_t readBuff[65536];
    uint8_t writeBuff[65536];
public:
    Transport(int sock, sockaddr_in* client);
    Transport(const std::string tragetIP, const int targetPort);
    ~Transport() {};
    int64_t sendMessage(Message* m);
    int64_t readMessage(Message* m, MQTT_ERROR& err);
};
#endif // MQTT_TRANSPORT_H_
