#pragma once

#include "can-codec.h"

struct ModelN : CanCodec {
private:
    ModelN() : CanCodec("modeln", 0x60, 0x02) {}

public:
    static const int BYTE_COUNT = 9;

    static const ModelN* instance() {
        static ModelN instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        char buff[BYTE_COUNT + 1];
        memcpy(buff, data, BYTE_COUNT);
        buff[BYTE_COUNT] = '\0';
        doc["data"]["modeln"] = buff;
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {}

};
