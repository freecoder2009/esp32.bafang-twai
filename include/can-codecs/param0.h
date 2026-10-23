#pragma once

#include "can-codec.h"

struct Param0 : CanCodec {
private:
    Param0() : CanCodec("param0", 0x60, 0x10) {}

public:
    static const int BYTE_COUNT = 64;

    static const Param0* instance() {
        static Param0 instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        int offset;
        doc["data"]["speedLim"] = data[0];
        for (int i = 0; i < 9; i++) {
            offset = 1 + i;
            doc["data"]["accel"][i] = data[offset];
        }
        for (int i = 0; i < 9; i++) {
            offset = 10 + i * 2;
            doc["data"]["assist"][i] =
                (uint16_t)data[offset] |
                ((uint16_t)data[offset + 1] << 8);
        }
        doc["data"]["assistUpLim"] =
            (uint16_t)data[28] |
            ((uint16_t)data[29] << 8);
        for (int i = 30; i <= 62; i++) {
            offset = i-30;
            doc["data"]["unknown"][offset] = data[i];
        }
        doc["data"]["checksum"] = data[63];
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {
        int offset;
        data[0] = doc["data"]["speedLim"].as<uint8_t>();
        for (int i = 0; i < 9; i++) {
            offset = 1 + i;
            data[offset] = doc["data"]["accel"][i].as<uint8_t>();
        }
        for (int i = 0; i < 9; i++) {
            uint16_t value = doc["data"]["assist"][i].as<uint16_t>();
            offset = 10 + i * 2;        
            data[offset]     = value & 0xFF;
            data[offset + 1] = (value >> 8) & 0xFF;
        }
        uint16_t assistUpLim = doc["data"]["assistUpLim"].as<uint16_t>();
        data[28] = assistUpLim & 0xFF;
        data[29] = (assistUpLim >> 8) & 0xFF;
        for (int i = 30; i <= 62; i++) {
            offset = i-30;
            data[i] = doc["data"]["unknown"][offset].as<uint8_t>();
        }
        data[63] = checksum(data, 63);
    }
};
