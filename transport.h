#ifndef MQTT_TRANSPORT_H_
#define MQTT_TRANSPORT_H_

#include <stdint.h>
#include "frame.h"

class Transport {
    int sock;
    struct sockaddr_in target;
    uint8_t readBuff[65535];
    uint8_t writeBuff[65535];
public:
    Transport(const std::string targetIP, int targetPort);
    ~Transport() {};
    int64_t sendMessage(Message* m);
    int64_t readMessage(Message* m);
};
#endif // MQTT_TRANSPORT_H_
