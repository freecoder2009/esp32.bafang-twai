#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "driver/twai.h"

struct CanCommand;
struct CanCodec;
struct CanMessage;
struct CanReadBuffer;
struct CanWriteBuffer;

void setupSerial();
void setupTwai();
bool readSerialLine();
bool requireCanUnlocked();
bool parseSerialLine();
bool lockForSupportedCmd();
bool canLockByCmd(const CanCommand *canCmd);
bool requireReadInit();
bool canSendMsg(
    uint32_t canId,
    uint8_t dlc=0,
    uint8_t data[]=nullptr
);
void buildCanMsg(
    twai_message_t *canMsg,
    uint32_t canId,
    uint8_t dlc=0,
    uint8_t data[]=nullptr
);
void prepareBuffer();
bool canReceiveMsg();
void decodeToBMessage(const twai_message_t &msg);
boolean isCtrlerToBsst();
bool decodeMf();
bool decodedMfStart();
bool decodedMfFrame();
bool sendReadAck();
void saveMfTotBytes();
void saveMfBytes();
bool isCompleteMfRead();
void returnBResult();
bool requireWriteInit();
bool canReceiveAck();
bool isAckOk();
bool isAckErrWarn();
bool isCompleteWriteOrCanWriteMf();
bool canWriteNextMf();
bool canLockedForTooLong();
void canUnlock();
void errorUnlock(const char* format, ...);
