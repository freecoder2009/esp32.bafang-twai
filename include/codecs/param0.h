#pragma once

#include "prototypes.h"
#include "types.h"

struct Param0 : CanAttr {

    Param0() : CanAttr("param0", 0x60, 0x10) {}

    int dataSize() const override {
        return 64;
    }

    void toJson(JsonDocument &doc, uint8_t data[]) const override {
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

    void fromJson(JsonDocument &doc, uint8_t data[]) const override {
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
        data[63] = 0;//checksum(data, 63);
    }

    private:
    uint8_t checksum(uint8_t data[], int byteCnt) const {
        uint16_t sum = 0;
        for (int i=0; i<byteCnt; i++) {
            sum += data[i];
        }
        return sum & 0xFF;
    }
};

const Param0 CAN_PARAM0;

const CanCommand CAN_PARAM0_READ  {
    .attr      = &CAN_PARAM0,
    .action    = "read",
    .op        = CAN_OP_READ,
    .rwStartId = bafangCan_getReadId(CAN_PARAM0),
    .ackId     = bafangCan_getReadAckId(CAN_PARAM0)
};

const CanCommand CAN_PARAM0_WRITE {
    .attr      = &CAN_PARAM0,
    .action    = "write",
    .op        = CAN_OP_WRITE,
    .rwStartId = bafangCan_getWriteStartId(CAN_PARAM0),
    .ackId     = bafangCan_getWriteAckId(CAN_PARAM0)
};
