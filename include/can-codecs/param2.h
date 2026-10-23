#pragma once

#include "can-codec.h"

struct Param2 : CanCodec {
private:
    Param2() : CanCodec("param2", 0x60, 0x12) {}

public:
    static const int BYTE_COUNT = 64;

    static const Param2* instance() {
        static Param2 instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        for (int i = 0; i < 6; i++) {
            doc["data"]["startTorque"][i] = data[i];
        }
        for (int i = 0; i < 6; i++) {
            doc["data"]["fullLoadTorque"][i] = data[6 + i];
        }
        for (int i = 0; i < 6; i++) {
            doc["data"]["returnTorque"][i] = data[12 + i];
        }
        for (int i = 0; i < 6; i++) {
            doc["data"]["maxCurrent"][i] = data[18 + i];
        }
        for (int i = 0; i < 6; i++) {
            doc["data"]["minCurrent"][i] = data[24 + i];
        }
        for (int i = 0; i < 6; i++) {
            doc["data"]["torqueAttenuation"][i] = data[30 + i];
        }
        for (int i = 0; i < 6; i++) {
            doc["data"]["startPulse"][i] = data[36 + i];
        }
        for (int i = 0; i < 6; i++) {
            doc["data"]["currentDecayTime"][i] = data[42 + i];
        }
        for (int i = 0; i < 15; i++) {
            doc["data"]["unknown"][i] = data[48 + i];
        }
        doc["data"]["checksum"] = data[63];
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {
        for (int i = 0; i < 6; i++) {
            data[i] = doc["data"]["startTorque"][i].as<uint8_t>();
        }
        for (int i = 0; i < 6; i++) {
            data[6 + i] = doc["data"]["fullLoadTorque"][i].as<uint8_t>();
        }
        for (int i = 0; i < 6; i++) {
            data[12 + i] = doc["data"]["returnTorque"][i].as<uint8_t>();
        }
        for (int i = 0; i < 6; i++) {
            data[18 + i] = doc["data"]["maxCurrent"][i].as<uint8_t>();
        }
        for (int i = 0; i < 6; i++) {
            data[24 + i] = doc["data"]["minCurrent"][i].as<uint8_t>();
        }
        for (int i = 0; i < 6; i++) {
            data[30 + i] = doc["data"]["torqueAttenuation"][i].as<uint8_t>();
        }
        for (int i = 0; i < 6; i++) {
            data[36 + i] = doc["data"]["startPulse"][i].as<uint8_t>();
        }
        for (int i = 0; i < 6; i++) {
            data[42 + i] = doc["data"]["currentDecayTime"][i].as<uint8_t>();
        }
        for (int i = 0; i < 15; i++) {
            data[48 + i] = doc["data"]["unknown"][i].as<uint8_t>();
        }
        data[63] = checksum(data, 63);
    }

};
