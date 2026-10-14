#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "prototypes.h"

const int MAX_RESULT_BUFF_LEN = 128;
const int MAX_RESULT_FRAME_CNT = MAX_RESULT_BUFF_LEN/8 + 1;

struct CanAttr {
  const char* name;
  uint8_t cmd;
  uint8_t subCmd;
  CanAttr(const char* n, uint8_t c, uint8_t s) : name(n), cmd(c), subCmd(s) {}
  virtual void toJson(JsonDocument &doc, uint8_t data[]) const = 0;
  virtual void fromJson(JsonDocument &doc, uint8_t data[]) const = 0;
  virtual int dataSize() const = 0;
  virtual ~CanAttr() = default;
};

struct CanCommand {
  const CanAttr* attr;
  const char* action;
  uint32_t id;
};

struct CanMessage {
    uint32_t id;
    uint8_t source;
    uint8_t target;
    uint8_t op;
    uint8_t cmd;
    uint8_t subCmd;
    uint8_t dlc;
    uint8_t data[8];
};

struct CanReadBuffer {
    int totBytes;
    int rcvdBytes;
    bool rcvdFrames[MAX_RESULT_FRAME_CNT];
    uint8_t data[MAX_RESULT_BUFF_LEN]; 
};

struct CanWriteBuffer {
    int totBytes;
    int sentBytes;
    bool sentFrames[MAX_RESULT_FRAME_CNT];
    uint8_t data[MAX_RESULT_BUFF_LEN];
};
