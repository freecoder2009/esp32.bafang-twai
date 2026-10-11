#pragma once

struct CanCommand;
struct CanAttr;

bool readSerialLine();
bool requireCanUnlocked();
bool parseSerialLine();
bool lockForSupportedCmd();
bool canLockByCmd(CanCommand canCmd);
bool canSendReadReq();
bool cleanReadBuffer();
bool loadWriteBuffer();
void prepareLockedCmd();
bool canReceiveMsg();
void decodeToBMessage(const twai_message_t &msg);
boolean isCtrlerToBsst();
bool decodedMf();
bool decodedMfStart();
bool decodedMfFrame();
bool sendAck();
void saveMfBytes();
bool completedMf();
void returnBResult();
void decodeCanData();
void decodeCanParam0();
bool canLockedForTooLong();
void canUnlock();
void errorUnlock(const char* format, ...);

uint32_t bafangCanId(const int source, const int target, const int op, int cmd, int subCmd);
uint32_t bafangCanReadId(const CanAttr &canAttr);
uint32_t bafangCanWriteId(const CanAttr &canAttr);
uint32_t bafangCanAckId(const CanAttr &canAttr);

