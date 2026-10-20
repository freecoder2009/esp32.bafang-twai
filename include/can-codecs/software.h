#pragma once

#include "can-codec.h"

struct Software : CanCodec {
private:
    Software() : CanCodec("software", 0x60, 0x01) {}

public:    
    static const Software* instance() {
        static Software instance;
        return &instance;
    }

    int byteCount() const override {
        return 20;
    }

    void bytesToJson(JsonDocument &doc, const uint8_t data[]) const override {}

    void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const override {}

};
