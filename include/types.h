#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "can-codec.h"
#include "can-command.h"

const int MAX_RESULT_BUFF_LEN = 128;
const int MAX_RESULT_FRAME_CNT = MAX_RESULT_BUFF_LEN/8 + 1;

struct CanMessage {
    uint32_t canId;
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

    void clean() {
        totBytes = -1;
        rcvdBytes = 0;
        for (int i=0 ; i<MAX_RESULT_BUFF_LEN ; i++) {
            data[i] = 0;
        }
        for (int i=0 ; i<MAX_RESULT_FRAME_CNT ; i++) {
            rcvdFrames[i] = false;
        }
    }
};

struct CanWriteBuffer {
    int totBytes;
    int sentBytes;
    int sentFrames;
    uint8_t data[MAX_RESULT_BUFF_LEN];

    void load(const JsonDocument &doc, const CanCodec *codec) {
        sentBytes = 0;
        sentFrames = 0;
        totBytes = codec->byteCount();
        codec->jsonToBytes(doc, data);
    }
};
