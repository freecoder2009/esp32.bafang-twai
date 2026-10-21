#pragma once

#include "can-codec.h"

struct Hardware : CanCodec {
private:
    Hardware() : CanCodec("hardware", 0x60, 0x00) {}

public:
    static const int BYTE_COUNT = 13;

    static const Hardware* instance() {
        static Hardware instance;
        return &instance;
    }

    int byteCount() const override {
        return BYTE_COUNT;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {
        char buff[BYTE_COUNT + 1];
        memcpy(buff, data, BYTE_COUNT);
        buff[BYTE_COUNT] = '\0';
        doc["data"]["hversion"] = buff;
    }

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {}

};
