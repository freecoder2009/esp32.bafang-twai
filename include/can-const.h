#pragma once

#include <Arduino.h>

namespace BafangCan {
    
    // CAN system component codes
    constexpr uint8_t CAN_DEV_TORQUE_SENSOR = 0x01;
    constexpr uint8_t CAN_DEV_DRIVE_UNIT = 0x02;
    constexpr uint8_t CAN_DEV_DISPLAY = 0x03;
    constexpr uint8_t CAN_DEV_BATTERY = 0x04;
    constexpr uint8_t CAN_DEV_BESST = 0x05;
    constexpr uint8_t CAN_DV_BROADCAST = 0x1F;

    // CAN operation codes
    constexpr uint8_t CAN_OP_WRITE = 0x00;
    constexpr uint8_t CAN_OP_READ = 0x01;
    constexpr uint8_t CAN_OP_NORMAL_ACK = 0x02;
    constexpr uint8_t CAN_OP_ERROR_ACK = 0x03;
    constexpr uint8_t CAN_OP_MULTIFRAME_START = 0x04;
    constexpr uint8_t CAN_OP_MULTIFRAME = 0x05;
    constexpr uint8_t CAN_OP_MULTIFRAME_END = 0x06;
    constexpr uint8_t CAN_OP_MULTIFRAME_WARNING = 0x07;
}
