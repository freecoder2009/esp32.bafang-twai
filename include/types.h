#pragma once

#include "prototypes.h"

const int MAX_RESULT_BUFF_LEN = 128;
const int MAX_RESULT_FRAME_CNT = MAX_RESULT_BUFF_LEN/8 + 1;

struct CanAttr {
  const char* name;
  uint8_t cmd;
  uint8_t subCmd;
};

struct CanCommand {
  const CanAttr* attr;
  const char* action;
  uint32_t id;
};

struct BafangMessage {
    uint32_t id;
    uint8_t source;
    uint8_t target;
    uint8_t op;
    uint8_t cmd;
    uint8_t subCmd;
    uint8_t dlc;
    uint8_t data[8];
};

struct BafangResult {
    const CanAttr* attr;
    int totBytes;
    int rcvdBytes;
    bool rcvdFrames[MAX_RESULT_FRAME_CNT];
    uint8_t data[MAX_RESULT_BUFF_LEN];
};