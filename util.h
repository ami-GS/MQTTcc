#ifndef MQTT_UTIL_H_
#define MQTT_UTIL_H_

#include <string>
#include <stdint.h>

int32_t UTF8_encode(uint8_t* wire, std::string s);

std::string UTF8_decode(uint8_t* wire, int64_t* len);

int32_t remainEncode(uint8_t* wire, uint32_t len);
int32_t remainDecode(uint8_t* wire, int* len);


#endif //MQTT_UTIL_H_
