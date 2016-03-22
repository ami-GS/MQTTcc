#include "frame.h"
#include "util.h"
#include "gtest/gtest.h"
#include <string>
#include <iostream>

TEST(UtilTest, NormalTest) {
    std::string data = "hello world";
    uint8_t enc_a_wire[30];
    const uint8_t e_wire[13] = {0x00, 0x0b, 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
    int32_t enc_a_len = UTF8_encode(enc_a_wire, data);
    int32_t enc_e_len = data.size()+2;
    EXPECT_EQ(enc_e_len, enc_a_len);
    EXPECT_TRUE(0 == std::memcmp(enc_a_wire, e_wire, enc_a_len));

    int64_t dec_a_len = 0;
    std::string dec_a_str = UTF8_decode(e_wire, &dec_a_len);
    EXPECT_EQ(enc_e_len, dec_a_len);
    EXPECT_TRUE(0 == std::memcmp(data.c_str(), dec_a_str.c_str(), data.size()));

    int dataNum = 8;
    uint32_t lens[8] = {0, 127, 128, 16383, 16384, 2097151, 2097152, 268435455};
    uint8_t remEnc_a_wire[30];
    uint8_t remEnc_e_wires[8][4] = {{0x00}, {0x7f}, {0x80, 0x01}, {0xff, 0x7f}, {0x80, 0x80, 0x01},
                                 {0xff, 0xff, 0x7f}, {0x80, 0x80, 0x80, 0x01}, {0xff, 0xff, 0xff, 0x7f}};
    for (int i = 0; i < dataNum; i++) {
        int32_t remEnc_e_len = remainEncode(remEnc_a_wire, lens[i]);
        EXPECT_TRUE(0 == std::memcmp(remEnc_e_wires[i], remEnc_a_wire, remEnc_e_len));
    }

    int wire_progress = 0;
    MQTT_ERROR err;
    for (int i = 0; i < dataNum; i++) {
        int32_t a_remLength = remainDecode(remEnc_e_wires[i], &wire_progress, err);
        EXPECT_EQ(lens[i], a_remLength);
    }

    std::vector<std::string> a_parts, e_parts;
    e_parts.push_back("a");
    e_parts.push_back("b");
    e_parts.push_back("c");
    std::string splitted = "a/b/c";
    int done = split(splitted, "/", &a_parts);
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(e_parts[i] == a_parts[i]);
    }
}

TEST(FrameTest, NormalTest) {
    MessageType type = CONNECT_MESSAGE_TYPE;
    bool dup = false;
    uint8_t qos = 0;
    bool retain = false;
    uint32_t len = 0;
    uint16_t id = 0;

    FixedHeader* fh = new FixedHeader(type, dup, qos, retain, len, id);
    EXPECT_EQ(type, fh->Type);
    EXPECT_EQ(dup, fh->Dup);
    EXPECT_EQ(qos, fh->QoS);
    EXPECT_EQ(retain, fh->Retain);
    EXPECT_EQ(len, fh->Length);
    EXPECT_EQ(id, fh->PacketID);

}
    

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
