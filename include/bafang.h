#pragma once

#include "prototypes.h"

// CAN system component codes
const int CAN_TORQUE_SENSOR = 0x01;
const int CAN_DRIVE_UNIT = 0x02;
const int CAN_DISPLAY = 0x03;
const int CAN_BATTERY = 0x04;
const int CAN_BESST = 0x05;
const int CAN_BROADCAST = 0x1F;

// CAN command codes
const int CAN_WRITE_CMD = 0x00;
const int CAN_READ_CMD = 0x01;
const int CAN_NORMAL_ACK = 0x02;
const int CAN_ERROR_ACK = 0x03;
const int CAN_MULTIFRAME_START = 0x04;
const int CAN_MULTIFRAME = 0x05;
const int CAN_MULTIFRAME_END = 0x06;
const int CAN_MULTIFRAME_WARNING = 0x07;

uint32_t bafangCanReadId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_BESST, 
        CAN_DRIVE_UNIT, 
        CAN_READ_CMD, 
        canAttr.cmd, 
        canAttr.subCmd
    );
}
uint32_t bafangCanWriteId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_BESST, 
        CAN_DRIVE_UNIT, 
        CAN_WRITE_CMD, 
        canAttr.cmd, 
        canAttr.subCmd
    );
}
uint32_t bafangCanAckId(const CanAttr &canAttr) {
    return bafangCanId(
        CAN_BESST, 
        CAN_DRIVE_UNIT, 
        CAN_NORMAL_ACK, 
        canAttr.cmd, 
        canAttr.subCmd
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
