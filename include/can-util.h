#pragma once

#include <Arduino.h>
#include "can-const.h"
#include "can-codec.h"

namespace BafangCan {

    uint32_t canGetCanId(const int source, const int target, const int op, int cmd, int subCmd) {
        uint8_t b0 = 0x80 | (source & 0x0F);
        uint8_t b1 = ((target & 0x1F) << 3) | (op & 0x07);
        return ((uint32_t)b0 << 24) |
            ((uint32_t)b1 << 16) |
            ((uint32_t)cmd << 8) |
            subCmd;
    }

    uint32_t canGetReadId(const CanCodec *canAttr) {
        return canGetCanId(
            CAN_DEV_BESST, CAN_DEV_DRIVE_UNIT, 
            CAN_OP_READ,
            canAttr->cmd, canAttr->subCmd
        );
    }

    uint32_t canGetReadAckId(const CanCodec *canAttr) {
        return canGetCanId(
            CAN_DEV_BESST, CAN_DEV_DRIVE_UNIT, 
            CAN_OP_NORMAL_ACK,
            canAttr->cmd, canAttr->subCmd
        );
    }

    uint32_t canGetWriteStartId(const CanCodec *canAttr) {
        return canGetCanId(
            CAN_DEV_BESST, CAN_DEV_DRIVE_UNIT,
            CAN_OP_WRITE,
            canAttr->cmd, canAttr->subCmd
        );
    }

    uint32_t canGetWriteAckId(const CanCodec *canAttr) {
        return canGetCanId(
            CAN_DEV_DRIVE_UNIT, CAN_DEV_BESST,
            CAN_OP_NORMAL_ACK,
            canAttr->cmd, canAttr->subCmd
        );
    }

    uint32_t canGetWriteStartFrameId(const CanCodec *canAttr) {
        return canGetCanId(
            CAN_DEV_BESST, CAN_DEV_DRIVE_UNIT,
            CAN_OP_MULTIFRAME_START,
            canAttr->cmd, canAttr->subCmd
        );
    }

    uint32_t canGetWriteFrameId(int frameIdx) {
        return canGetCanId(
            CAN_DEV_BESST, CAN_DEV_DRIVE_UNIT,
            CAN_OP_MULTIFRAME,
            0, frameIdx
        );
    }

    uint32_t canGetWriteEndFrameId(int frameIdx) {
        return canGetCanId(
            CAN_DEV_BESST, CAN_DEV_DRIVE_UNIT,
            CAN_OP_MULTIFRAME_END,
            0, frameIdx
        );
    }
}
