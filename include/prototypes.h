#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "driver/twai.h"

struct CanCommand;
struct CanAttr;
struct CanMessage;
struct CanReadBuffer;
struct CanWriteBuffer;

struct Param0;

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
void cleanReadBuffer();
bool loadWriteBuffer();
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
bool canLockedForTooLong();
void canUnlock();
void errorUnlock(const char* format, ...);

uint32_t bafangCan_getReadId(const CanAttr &canAttr);
uint32_t bafangCan_getReadAckId(const CanAttr &canAttr);
uint32_t bafangCan_getWriteStartId(const CanAttr &canAttr);
uint32_t bafangCan_getWriteAckId(const CanAttr &canAttr);
uint32_t bafangCan_getWriteStartFrameId(const CanAttr &canAttr);
uint32_t bafangCan_getWriteFrameId(int frmIdx, bool isLast);
uint32_t bafangCanId(const int source, const int target, const int op, int cmd, int subCmd);