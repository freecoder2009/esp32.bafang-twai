#pragma once

#include "can-codec.h"

struct SerialN : CanCodec {
private:
    SerialN() : CanCodec("serialn", 0x60, 0x03) {}

public:
    static const int BYTE_COUNT = 22;

    static const SerialN* instance() {
        static SerialN instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        char buff[BYTE_COUNT + 1];
        memcpy(buff, data, BYTE_COUNT);
        buff[BYTE_COUNT] = '\0';
        doc["data"]["serialn"] = buff;
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {}

};
