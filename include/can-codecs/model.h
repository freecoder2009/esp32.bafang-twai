#pragma once

#include "can-codec.h"

struct Model : CanCodec {
private:
    Model() : CanCodec("model", 0x60, 0x02) {}

public:
    static const int BYTE_COUNT = 9;

    static const Model* instance() {
        static Model instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        char buff[BYTE_COUNT + 1];
        memcpy(buff, data, BYTE_COUNT);
        buff[BYTE_COUNT] = '\0';
        doc["data"]["modelv"] = buff;
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {}

};
