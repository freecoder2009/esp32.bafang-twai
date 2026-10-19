#pragma once

#include "prototypes.h"

// CAN system component codes
const int CAN_DEV_TORQUE_SENSOR = 0x01;
const int CAN_DEV_DRIVE_UNIT = 0x02;
const int CAN_DEV_DISPLAY = 0x03;
const int CAN_DEV_BATTERY = 0x04;
const int CAN_DEV_BESST = 0x05;
const int CAN_DV_BROADCAST = 0x1F;

// CAN operation codes
const int CAN_OP_WRITE = 0x00;
const int CAN_OP_READ = 0x01;
const int CAN_OP_NORMAL_ACK = 0x02;
const int CAN_OP_ERROR_ACK = 0x03;
const int CAN_OP_MULTIFRAME_START = 0x04;
const int CAN_OP_MULTIFRAME = 0x05;
const int CAN_OP_MULTIFRAME_END = 0x06;
const int CAN_OP_MULTIFRAME_WARNING = 0x07;

uint32_t bafangCan_getReadId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_DEV_BESST, 
        CAN_DEV_DRIVE_UNIT, 
        CAN_OP_READ, 
        canAttr.cmd, 
        canAttr.subCmd
    );
}
uint32_t bafangCan_getReadAckId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_DEV_BESST, 
        CAN_DEV_DRIVE_UNIT, 
        CAN_OP_NORMAL_ACK, 
        canAttr.cmd, 
        canAttr.subCmd
    );
}
uint32_t bafangCan_getWriteStartId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_DEV_BESST,
        CAN_DEV_DRIVE_UNIT,
        CAN_OP_WRITE,
        canAttr.cmd, 
        canAttr.subCmd
    );
}
uint32_t bafangCan_getWriteAckId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_DEV_DRIVE_UNIT,
        CAN_DEV_BESST,
        CAN_OP_NORMAL_ACK, 
        canAttr.cmd, 
        canAttr.subCmd
    );
}
uint32_t bafangCan_getWriteStartFrameId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_DEV_BESST,
        CAN_DEV_DRIVE_UNIT,
        CAN_OP_MULTIFRAME_START,
        canAttr.cmd, 
        canAttr.subCmd
    );
}
uint32_t bafangCan_getWriteFrameId(int frameIdx) {
    return bafangCanId(
        CAN_DEV_BESST,
        CAN_DEV_DRIVE_UNIT,
        CAN_OP_MULTIFRAME,
        0, frameIdx
    );
}
uint32_t bafangCan_getWriteEndFrameId(int frameIdx) {
    return bafangCanId(
        CAN_DEV_BESST,
        CAN_DEV_DRIVE_UNIT,
        CAN_OP_MULTIFRAME_END,
        0, frameIdx
    );
}
uint32_t bafangCanId(const int source, const int target, const int op, int cmd, int subCmd) {
    uint8_t b0 = 0x80 | (source & 0x0F);
    uint8_t b1 = ((target & 0x1F) << 3) | (op & 0x07);
    return ((uint32_t)b0 << 24) |
           ((uint32_t)b1 << 16) |
           ((uint32_t)cmd << 8) |
           subCmd;
}
