// system component codes, usable as CAN message source/target
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

struct CanCommand {
  const char* name;
  uint32_t id;
};

const CanCommand CAN_PARAM0_READ { .name="param0.read", .id=0x85116010 };
const CanCommand CAN_PARAM1_READ { .name="param1.read", .id=0x85116011 };

const CanCommand CAN_SUPPORTED_COMMANDS[] = {
  CAN_PARAM0_READ
};

