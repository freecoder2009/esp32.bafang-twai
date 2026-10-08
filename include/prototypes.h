#pragma once

struct CanCommand;

bool readSerialLine();
bool requireCanUnlocked();
bool parseSerialLine();
bool lockForSupportedCommand();
bool lockForReq(CanCommand canCmd);
bool canSendReq(CanCommand canCmd);
void initBResult();
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
void canLock();
void canUnlock();
void errorUnlock(const char* format, ...);
