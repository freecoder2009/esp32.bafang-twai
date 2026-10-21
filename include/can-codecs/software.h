#pragma once

#include "can-codec.h"

struct Software : CanCodec {
private:
    Software() : CanCodec("software", 0x60, 0x01) {}

public:
    static const int BYTE_COUNT = 20;

    static const Software* instance() {
        static Software instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        char buff[BYTE_COUNT + 1];
        memcpy(buff, data, BYTE_COUNT);
        buff[BYTE_COUNT] = '\0';
        doc["data"]["firmware"] = buff;
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {}

};
