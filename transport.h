#ifndef MQTT_TRANSPORT_H_
#define MQTT_TRANSPORT_H_

#include <stdint.h>
#include <netinet/in.h>
#include "frame.h"
#include "mqttError.h"

class Transport {
    struct sockaddr_in* target;
public:
    uint8_t readBuff[65536];
    uint8_t writeBuff[65536];
    int sock;
    Transport(int sock, sockaddr_in* client);
    Transport(const std::string tragetIP, const int targetPort);
    ~Transport() {};
    int64_t sendMessage(Message* m);
    MQTT_ERROR readMessage();
};


#endif // MQTT_TRANSPORT_H_
