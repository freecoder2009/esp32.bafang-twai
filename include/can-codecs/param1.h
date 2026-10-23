#pragma once

#include "can-codec.h"

struct Param1 : CanCodec {
private:
    Param1() : CanCodec("param1", 0x60, 0x11) {}

public:
    static const int BYTE_COUNT = 64;

    static const Param1* instance() {
        static Param1 instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        for (int i = 0; i < 40; i++) {
            doc["data"]["unknown1"][i] = data[i];
        }
        for (int i = 0; i < 9; i++) {
            doc["data"]["pasCurrentLim"][i] = data[40 + i];
        }
        for (int i = 0; i < 9; i++) {
            doc["data"]["pasSpeedLim"][i] = data[49 + i];
        }
        for (int i = 0; i < 5; i++) {
            doc["data"]["unknown2"][i] = data[58 + i];
        }
        doc["data"]["checksum"] = data[63];
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {
        for (int i = 0; i < 40; i++) {
            data[i] = doc["data"]["unknown1"][i].as<uint8_t>();
        }
        for (int i = 0; i < 9; i++) {
            data[40 + i] = doc["data"]["pasCurrentLim"][i].as<uint8_t>();
        }
        for (int i = 0; i < 9; i++) {
            data[49 + i] = doc["data"]["pasSpeedLim"][i].as<uint8_t>();
        }
        for (int i = 0; i < 5; i++) {
            data[58 + i] = doc["data"]["unknownAfter"][i].as<uint8_t>();
        }
        data[63] = checksum(data, 63);
    }

};
