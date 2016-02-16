#ifndef MQTT_TRANSPORT_H_
#define MQTT_TRANSPORT_H_

#include <stdint.h>
#include "frame.h"

int64_t sendMessage(Message* m);
int64_t readMessage(Message* m);

#endif // MQTT_TRANSPORT_H_
