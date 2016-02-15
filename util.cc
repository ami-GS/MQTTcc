#include "util.h"
#include <string>
#include <stdint.h>


int32_t UTF8_encode(uint8_t* wire, std::string s) {
    uint8_t* buf = wire;
    *buf = (uint8_t)(s.size() >> 8);
    *(++buf) = (uint8_t)(s.size());
    std::memcpy(++buf, s.c_str(), s.size());
    return (int32_t)(2 + s.size());
}

std::string UTF8_decode(uint8_t* wire, int64_t* len) {
    *len = (uint16_t)*wire << 8;
    *len |= *(wire+1);
    std::string s(*len, '0');
    for (int i = 0; i < *len; i++) {
        s[i] = *(wire+2+i);
    }
    *len += 2;
    return s;
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

int32_t remainDecode(uint8_t* wire, int* len) {
    uint8_t* buf = wire;
    uint32_t m = 1;
    uint32_t out = 0;
    for ( ; ; buf++) {
        out += ((*buf)&0x7f) * m;
        m *= 0x80;
        if ((*buf&0x80) == 0) {
            break;
        }
        if (m > 2097152) {
            return -1;
        }
    }
    *len = buf - wire;
    return out;
}
