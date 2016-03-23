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

    int dataNum = 9;
    uint32_t lens[9] = {0, 1, 127, 128, 16383, 16384, 2097151, 2097152, 268435455};
    uint8_t remEnc_a_wire[30];
    uint8_t remEnc_e_wires[9][4] = {{0x00}, {0x01}, {0x7f}, {0x80, 0x01}, {0xff, 0x7f}, {0x80, 0x80, 0x01},
                                 {0xff, 0xff, 0x7f}, {0x80, 0x80, 0x80, 0x01}, {0xff, 0xff, 0xff, 0x7f}};
    uint8_t e_wire_len[9] = {1, 1, 1, 2, 2, 3, 3, 4, 4};
    for (int i = 0; i < dataNum; i++) {
        int32_t remEnc_e_len = remainEncode(remEnc_a_wire, lens[i]);
        EXPECT_TRUE(0 == std::memcmp(remEnc_e_wires[i], remEnc_a_wire, remEnc_e_len));
    }

    int wire_progress = 0;
    MQTT_ERROR err;
    for (int i = 0; i < dataNum; i++) {
        int32_t a_remLength = remainDecode(remEnc_e_wires[i], &wire_progress, err);
        EXPECT_EQ(e_wire_len[i], wire_progress);
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

TEST(FrameHeaderTest, NormalTest) {
    MessageType type = PUBLISH_MESSAGE_TYPE;
    bool dup = true;
    uint8_t qos = 2;
    bool retain = true;
    uint32_t len = 1;
    uint16_t id = 0;

    FixedHeader* fh = new FixedHeader(type, dup, qos, retain, len, id);
    EXPECT_EQ(type, fh->Type);
    EXPECT_EQ(dup, fh->Dup);
    EXPECT_EQ(qos, fh->QoS);
    EXPECT_EQ(retain, fh->Retain);
    EXPECT_EQ(len, fh->Length);
    EXPECT_EQ(id, fh->PacketID);

    uint8_t e_wire[2] = {0x3d, 0x01};
    int e_len = 2;
    uint8_t a_wire[30];
    int64_t a_len = fh->GetWire(a_wire);
    EXPECT_EQ(e_len, a_len);
    EXPECT_TRUE(0 == std::memcmp(e_wire, a_wire, e_len));

    MQTT_ERROR a_err = NO_ERROR;
    FixedHeader* a_fh = new FixedHeader();
    a_len = a_fh->parseHeader(e_wire, a_err);
    EXPECT_EQ(type, a_fh->Type);
    EXPECT_EQ(dup, a_fh->Dup);
    EXPECT_EQ(qos, a_fh->QoS);
    EXPECT_EQ(retain, a_fh->Retain);
    EXPECT_EQ(len, a_fh->Length);
    EXPECT_EQ(id, a_fh->PacketID);
    EXPECT_EQ(NO_ERROR, a_err);
    EXPECT_EQ(e_len, a_len);
}
    

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
