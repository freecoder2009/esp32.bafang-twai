#pragma once

#include "prototypes.h"
#include "types.h"

struct Software : CanAttr {

    Software() : CanAttr("software", 0x60, 0x01) {}

    void toJson(JsonDocument &doc, uint8_t data[]) const override {
    }

    void fromJson(JsonDocument &doc, uint8_t data[]) const override {
    }

    int dataSize() const override {
        return 0;
    }
};

const Software CAN_SOFTWARE;

const CanCommand CAN_SOFTWARE_READ  {
    .attr      = &CAN_SOFTWARE,
    .action    = "read",
    .op        = CAN_OP_READ,
    .canId = bafangCan_getReadId(CAN_SOFTWARE),
    .ackId     = bafangCan_getReadAckId(CAN_SOFTWARE)
};
