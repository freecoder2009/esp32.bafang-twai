#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct CanCodec {
    const char* name;
    const uint8_t cmd;
    const uint8_t subCmd;

    CanCodec(const char* name, uint8_t cmd, uint8_t subCmd)
        : name(name), cmd(cmd), subCmd(subCmd) {}

    virtual int byteCount() const = 0;
    virtual void bytesToJson(JsonDocument &doc, const uint8_t data[]) const = 0;
    virtual void jsonToBytes(const JsonDocument &doc, uint8_t data[]) const = 0;
    
    virtual ~CanCodec() = default;
    CanCodec(const CanCodec&) = delete;
    CanCodec& operator=(const CanCodec&) = delete;

protected:
    uint8_t checksum(uint8_t data[], int byteCnt) const {
        uint16_t sum = 0;
        for (int i=0; i<byteCnt; i++) {
            sum += data[i];
        }
        return sum & 0xFF;
    }
};