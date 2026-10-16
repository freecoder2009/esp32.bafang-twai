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
bool requireReadRequest();
void twaiMsg(
    twai_message_t *canMsg,
    uint32_t id,
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
bool sendAck();
void saveMfTotBytes();
void saveMfBytes();
bool isCompleteMfRead();
void returnBResult();
bool requireStartMf();
bool canReceiveAck();
bool isCompleteOrWriteMf();
bool canLockedForTooLong();
void canUnlock();
void errorUnlock(const char* format, ...);

uint32_t bafangCanReadId(const CanAttr &canAttr);
uint32_t bafangCanWriteId(const CanAttr &canAttr);
uint32_t bafangCanReadAckId(const CanAttr &canAttr);
uint32_t bafangCanWriteAckId(const CanAttr &canAttr);
uint32_t bafangCanId(const int source, const int target, const int op, int cmd, int subCmd);