#include "util.h"
#include "mqttError.h"
#include <string.h>
#include <stdint.h>


int32_t UTF8_encode(uint8_t* wire, std::string s) {
    uint8_t* buf = wire;
    *buf = (uint8_t)(s.size() >> 8);
    *(++buf) = (uint8_t)(s.size());
    memcpy(++buf, s.c_str(), s.size());
    return (int32_t)(2 + s.size());
}

int64_t UTF8_decode(const uint8_t* wire, std::string* s) {
    int64_t len = (uint16_t)*wire << 8;
    len |= *(wire+1);
    s->resize(len);
    for (int i = 0; i < len; i++) {
        (*s)[i] = *(wire+2+i);
    }
    return len+2;
}


int32_t remainEncode(uint8_t* wire, uint32_t len) {
    uint8_t* buf = wire;
    uint8_t digit = 0;
    while (len > 0) {
        digit = (uint8_t)(len % 128);
        len /= 128;
        if (len > 0) {
                digit |= 0x80;
            }
        *buf++ = digit;
    }
    return buf - wire;
}

int32_t remainDecode(const uint8_t* wire, int* len, MQTT_ERROR& err) {
    const uint8_t* buf = wire;
    uint32_t m = 1;
    uint32_t out = 0;
    for ( ; ; buf++) {
        out += ((*buf)&0x7f) * m;
        m *= 0x80;
        if ((*buf&0x80) == 0) {
            break;
        }
        if (m > 2097152) {
            err = MALFORMED_REMAIN_LENGTH;
            return -1;
        }
    }
    *len = buf - wire + 1;
    return out;
}

int split(std::string str, std::string sub, std::vector<std::string>* parts) {
  if (sub.size() != 1) {
    return -1; // the sub should be one charactor
  }
  size_t current = 0, found;
  while((found = str.find_first_of(sub, current)) != std::string::npos){
    parts->push_back(std::string(str, current, found - current));
    current = found + 1;
  }
  parts->push_back(std::string(str, current, found - current));
  return 1;
}
