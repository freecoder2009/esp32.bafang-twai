#include <Arduino.h>
#include <ArduinoJson.h>
#include "driver/twai.h"
#include "prototypes.h"
#include "types.h"
#include "logging.h"
#include "can-supported.h"

using namespace BafangCan;

const int LOG_LEVEL = LOG_DEBUG;

#define TX_PIN GPIO_NUM_5
#define RX_PIN GPIO_NUM_4

const unsigned long SERIAL_BAUD = 1000000;

const unsigned int MAX_SERIAL_BUFF_LEN = 512;
const unsigned long MAX_CAN_LOCK_MILLIS = 2000;
const int MAX_BYTES_PER_FRAME = 8;
const int DELAY_FRAME_WRITE_MS = 50;

char serialBuff[MAX_SERIAL_BUFF_LEN];
bool canLockedRead = false;
bool canLockedWrite = false;
bool readInitSent = false;
bool writeRequestSent = false;
const CanCommand *canLockedForCmd = nullptr;
unsigned long canLockedAt = 0;
CanReadBuffer bReadBuffer{};
CanWriteBuffer bWriteBuffer{};
CanMessage bMessage{};

JsonDocument doc;

void setup() {
    setupSerial();
    setupTwai();
}

void loop() {
    if(readSerialLine()
        && requireCanUnlocked()
        && parseSerialLine()
        && lockForSupportedCmd()
    ) {
        prepareBuffer();
    }
    if(canLockedRead
        && requireReadInit()
        && canReceiveMsg()
        && decodeMf()
        && isCompleteMfRead()
    ) {
        returnBResult();
        canUnlock();
    }
    if(canLockedWrite
        && requireWriteInit()
        && canReceiveAck()
        && isCompleteWriteOrCanWriteMf()
    ) {
        canUnlock();
    }
    if(canLockedForTooLong()) {
        errorUnlock("Timeout, unlocking.");
    }
}

void setupSerial() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial);
    Serial.println();
}

void setupTwai() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS(); // Bafang uses 250kbps
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        info("TWAI (CAN) Driver Installed Successfully.");
    } else {
        error("Failed to install driver.");
        return;
    }
    if (twai_start() == ESP_OK) {
        info("TWAI (CAN) Driver Started.");
    } else {
        error("Failed to start driver.");
    }
}

bool readSerialLine() {
  char c;
  int idx = 0;
  while(Serial.available()) {
    c = Serial.read();
    if(c == '\r') {
      serialBuff[idx] = '\0';
      idx = 0;
      return true;
    }
    if(idx < MAX_SERIAL_BUFF_LEN-1) {
      serialBuff[idx++] = c;
    }
  }
  return false;
}

bool requireCanUnlocked() {
    if(canLockedRead || canLockedWrite) {
        warn("CAN is already locked.");
        return false;
    }
    return true;
}

bool parseSerialLine() {
    DeserializationError jsonParseError = deserializeJson(doc, serialBuff);
    if (jsonParseError) {
        error("Error deserializing: %s", jsonParseError.c_str());
        return false;
    }
    return true;
}

bool lockForSupportedCmd() {
    if(canLockedRead || canLockedWrite) {
        return false;
    }
    for(int i=0 ; i<CAN_SUPPORTED_COUNT ; i++) {
        if(!canLockByCmd(&CAN_SUPPORTED[i])) {
            continue;
        }
        debug("Locked for request: %s.", serialBuff);
        return true;
    }
    error("Unrecognized request: %s.", serialBuff);
    return false;
}

bool canLockByCmd(const CanCommand *canCmd) {
    if(canLockedRead || canLockedWrite) {
        return false;
    }
    if(!doc["type"] || strcmp("req", doc["type"]) != 0) {
        return false;
    }
    if(!doc["field"] || strcmp(canCmd->codec->name, doc["field"]) != 0) {
        return false;
    }
    if(!doc["action"] || strcmp(canCmd->action, doc["action"]) != 0) {
        return false;
    }
    if(strcmp(canCmd->action, "read") == 0) {
        canLockedRead = true;
        canLockedAt = millis();
        canLockedForCmd = canCmd;
        return true;
    }
    if(strcmp(canCmd->action, "write") == 0) {
        canLockedWrite = true;
        canLockedAt = millis();
        canLockedForCmd = canCmd;
        return true;
    }
    return false;
}

void prepareBuffer() {
    if(canLockedRead) {
        bReadBuffer.clean();
        debug("Read-Buffer cleaned.");
    }
    if(canLockedWrite) {
        bWriteBuffer.load(doc, canLockedForCmd->codec);
        debugWriteBuffer("Write-Buffer loaded",
            canLockedForCmd, &bWriteBuffer);
    }
}

bool requireReadInit() {
    if(!canLockedRead) {
        error("Not locked for read.");
        return false;
    }
    if(readInitSent) {
        return true;
    }
    uint32_t canId = canGetReadId(canLockedForCmd->codec);
    debug("Sending Read-Init message (canId=0x%X).", canId);
    if(!canSendMsg(canId)) {
        return false;
    }
    readInitSent = true;
    return true;
}

bool canReceiveMsg() {
    if(!canLockedRead && !canLockedWrite) {
        return false;
    }
    twai_message_t message;
    esp_err_t res = twai_receive(&message, pdMS_TO_TICKS(100));
    if (res != ESP_OK) {
        errorUnlock("Failed receiving from CAN: %d", res);
        return false;
    }
    decodeToBMessage(message);
    if(!isCtrlerToBsst()) {
      return false;
    }
    logBMessage(LOG_TRACE, "Received CAN message", &bMessage);
    return true;
}

void decodeToBMessage(const twai_message_t &msg) {
    bMessage.canId = msg.identifier;
    bMessage.source = ((msg.identifier >> 24) & 0xFF) & 0x0F;
    bMessage.target = (msg.identifier >> 19) & 0x1F;
    bMessage.op     = (msg.identifier >> 16) & 0x07;
    bMessage.cmd    = (msg.identifier >> 8) & 0xFF;
    bMessage.subCmd = msg.identifier & 0xFF;
    bMessage.dlc = msg.data_length_code;
    for (uint8_t i = 0; i < bMessage.dlc && i < 8; i++) {
        bMessage.data[i] = msg.data[i];
    }
}

bool canSendMsg(
    uint32_t canId,
    uint8_t dlc,
    uint8_t data[]
) {
    if(!canLockedRead && !canLockedWrite) {
        return false;
    }
    twai_message_t canMsg = {};
    buildCanMsg(&canMsg, canId, dlc, data);
    traceCanMsg("Sending CAN message", &canMsg);
    esp_err_t res = twai_transmit(&canMsg, pdMS_TO_TICKS(1000));
    if (res != ESP_OK) {
        errorUnlock("Failed sending CAN message (canId=0x%X): %d", canId, res);
        return false;
    }
    return true;
}

boolean isCtrlerToBsst() {
    return (
        bMessage.source == CAN_DEV_DRIVE_UNIT &&
        bMessage.target == CAN_DEV_BESST
    );
}

bool decodeMf() {
    if(!canLockedRead) {
        return false;
    }
    if(decodedMfStart() || decodedMfFrame()) {
        return true;
    }
    return false;
}

bool decodedMfStart() {
    if(!canLockedRead) {
        return false;
    }
    if(
        bMessage.op != CAN_OP_MULTIFRAME_START ||
        bMessage.cmd != canLockedForCmd->codec->cmd ||
        bMessage.subCmd != canLockedForCmd->codec->subCmd
    ) {
        return false;
    }
    logBMessage(LOG_DEBUG, "Received MultiFrameStart", &bMessage);
    if(!sendReadAck()) {
        return false;
    }
    saveMfTotBytes();
    debug("Total bytes expected = %d", bMessage.data[0]);
    return true;
}

bool decodedMfFrame() {
    if(!canLockedRead) {
        return false;
    }
    if(bMessage.op == CAN_OP_MULTIFRAME) {
        logBMessage(LOG_DEBUG, "Received MultiFrame", &bMessage);
    } else if (bMessage.op == CAN_OP_MULTIFRAME_END) {
        logBMessage(LOG_DEBUG, "Received MultiFrameEnd", &bMessage);
    } else {
        return false;
    }
    if(!sendReadAck()) {
        return false;
    }
    saveMfBytes();
    debug("Total bytes saved/expected = %d/%d.",
        bReadBuffer.rcvdBytes, bReadBuffer.totBytes);
    return true;
}

bool sendReadAck() {
    if(!canLockedRead) {
        return false;
    }
    uint32_t canId = canGetReadAckId(canLockedForCmd->codec);
    trace("Sending Read-Ack message (canId=0x%X).", canId);
    if(!canSendMsg(canId)) {
        return false;
    }
    return true;
}

void saveMfTotBytes() {
    bReadBuffer.totBytes = bMessage.data[0];
}

void saveMfBytes() {
    uint8_t frmIdx = bMessage.subCmd;
    if(bReadBuffer.rcvdFrames[frmIdx]) {
        warn("Duplicate frame #%d received from CAN, ignoring.", frmIdx);
        return;
    }
    bReadBuffer.rcvdFrames[frmIdx] = true;
    for (int i=0 ; i<bMessage.dlc ; i++) {
        bReadBuffer.data[frmIdx*8 + i] = bMessage.data[i];
    }
    bReadBuffer.rcvdBytes += bMessage.dlc;
}

bool isCompleteMfRead() {
    return (canLockedRead &&
        bReadBuffer.totBytes > 0 &&
        bReadBuffer.rcvdBytes == bReadBuffer.totBytes
    );
}

void returnBResult() {
    debugReadBuffer("Returning", canLockedForCmd, &bReadBuffer);
    doc.clear();
    doc["type"] = "result";
    doc["field"] = canLockedForCmd->codec->name;
    doc["cmd"] = canLockedForCmd->codec->cmd;
    doc["subCmd"] = canLockedForCmd->codec->subCmd;
    canLockedForCmd->codec->bytesToJson(doc, bReadBuffer.data);
    serializeJson(doc, Serial);
    Serial.println();
}

bool requireWriteInit() {
    if(!canLockedWrite) {
        return false;
    }
    if(writeRequestSent) {
        return true;
    }
    uint32_t canId = canGetWriteStartId(canLockedForCmd->codec);
    uint8_t byteCnt = canLockedForCmd->codec->byteCount();
    uint8_t bytes[] = { byteCnt };
    debug("Sending Write-Init message (canId=0x%X, byteCnt=%d).", canId, byteCnt);
    if(!canSendMsg(canId, 1, bytes)) {
        return false;
    }
    writeRequestSent = true;
    return true;
}

bool canReceiveAck() {
    if(!canLockedWrite) {
        return false;
    }
    if(!canReceiveMsg()) {
        return false;
    }
    if(isAckErrWarn()) {
        errorUnlock("Error/Warning ack, unlocking.");
        return false;
    }
    if(!isAckOk()) {
        return false;
    }
    return true;
}

bool isAckOk() {
    return (bMessage.op == CAN_OP_NORMAL_ACK
        && bMessage.cmd == canLockedForCmd->codec->cmd
        && bMessage.subCmd == canLockedForCmd->codec->subCmd);
}

bool isAckErrWarn() {
    return (bMessage.op == CAN_OP_ERROR_ACK
        || bMessage.op == CAN_OP_MULTIFRAME_WARNING);
}

bool isCompleteWriteOrCanWriteMf() {
    if(!canLockedWrite) {
        return false;
    }
    if(bWriteBuffer.sentBytes == bWriteBuffer.totBytes) {
        debug("Write complete for %s; bytes tot/sent %d/%d.", 
            canLockedForCmd->codec->name, bWriteBuffer.totBytes, bWriteBuffer.sentBytes);
        return true;
    }
    canWriteNextMf();
    return false;
}

bool canWriteNextMf() {
    int remaining = bWriteBuffer.totBytes - bWriteBuffer.sentBytes;
    int dlc = (remaining < MAX_BYTES_PER_FRAME
        ? remaining
        : MAX_BYTES_PER_FRAME);
    int payloadCnt = bWriteBuffer.sentFrames + 1;
    int frameIdx = bWriteBuffer.sentFrames - 1;
    bool isStartFrame = (bWriteBuffer.sentBytes == 0);
    bool isEndFrame = (remaining == dlc);
    uint32_t canId = 
        (isStartFrame ? canGetWriteStartFrameId(canLockedForCmd->codec) :
        (isEndFrame   ? canGetWriteEndFrameId(frameIdx) :
        canGetWriteFrameId(frameIdx)));
    uint8_t *bOffset = bWriteBuffer.data + bWriteBuffer.sentBytes;
    const char *frmType = (isStartFrame ? "Start" : (isEndFrame ? "End" : "Frame"));

    // debug
    char data[dlc*3 + 10];
    for (int i=0; i<dlc; i++) {
        sprintf(data+3*i, "%02X ", bOffset[i]);
    }
    debug("Sending Mf%s Payload #%d, Frame idx=%d%s, canId=%08lX, tot/sent/rem/dlc %d/%d/%d/%d, sending now: %s",
        frmType, payloadCnt, (isStartFrame ? -1 : frameIdx), (isStartFrame ? "(N/A)" : ""), 
        canId, bWriteBuffer.totBytes, bWriteBuffer.sentBytes, remaining, dlc, data);
    // end debug
    
    if(!canSendMsg(canId, dlc, bOffset)) {
        return false;
    }
    bWriteBuffer.sentBytes += dlc;
    bWriteBuffer.sentFrames++;
    return true;
}

void buildCanMsg(
    twai_message_t *canMsg,
    uint32_t canId,
    uint8_t dlc,
    uint8_t data[]
) { 
    canMsg->identifier = canId;
    canMsg->extd = 1;
    canMsg->rtr = 0;
    canMsg->data_length_code = dlc;
    if(dlc > 0) {
        for (int i=0 ; i<dlc ; i++) {
            canMsg->data[i] = data[i];
        }
    }
}

bool canLockedForTooLong() {
    return (canLockedRead || canLockedWrite) && 
        (millis() - canLockedAt >= MAX_CAN_LOCK_MILLIS);
}

void errorUnlock(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LOG_ERROR, format, args);
    va_end(args);
    canUnlock();
}

void canUnlock() {
    canLockedRead = false;
    readInitSent = false;
    writeRequestSent = false;
    canLockedWrite = false;
    canLockedForCmd = nullptr;
    canLockedAt = 0;
    debug("System unlocked.");
}
