#include <Arduino.h>
#include <ArduinoJson.h>
#include "driver/twai.h"
#include "prototypes.h"
#include "types.h"
#include "bafang.h"
#include "codecs.h"
#include "logging.h"

const int LOG_LEVEL = LOG_DEBUG;

#define TX_PIN GPIO_NUM_5
#define RX_PIN GPIO_NUM_4

const unsigned long SERIAL_BAUD = 1000000;

const int MAX_SERIAL_BUFF_LEN = 512;
const unsigned long MAX_CAN_LOCK_MILLIS = 2000;

char serialBuff[MAX_SERIAL_BUFF_LEN];
bool canLockedRead = false;
bool canLockedWrite = false;
bool readRequestSent = false;
const CanCommand *canLockedForCmd = nullptr;
unsigned long canLockedAt = 0;
CanReadBuffer bReadBuffer{};
CanWriteBuffer bWriteBuffer{};
CanMessage bMessage{};

JsonDocument doc;

const CanCommand CAN_SUPPORTED_COMMANDS[] = {
    CAN_PARAM0_READ, CAN_PARAM0_WRITE
};
const int CAN_SUPPORTED_COMMANDS_COUNT = 
    sizeof(CAN_SUPPORTED_COMMANDS)
    / sizeof(CAN_SUPPORTED_COMMANDS[0]);

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
        && requireReadRequest()
        && canReceiveMsg()
        && decodeMf()
        && isCompleteMfRead()
    ) {
        returnBResult();
        canUnlock();
    }
    if(canLockedWrite
        && requireStartMf()
        && canReceiveAck()
        && isCompleteOrWriteMf()
    ) {
        canUnlock();
    }
    if(canLockedForTooLong()) {
        errorUnlock("Timeout while locked; aborting, unlocking.");
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
    for(int i=0 ; i<CAN_SUPPORTED_COMMANDS_COUNT ; i++) {
        if(!canLockByCmd(&CAN_SUPPORTED_COMMANDS[i])) {
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
    if(!doc["field"] || strcmp(canCmd->attr->name, doc["field"]) != 0) {
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
        cleanReadBuffer();
    }
    if(canLockedWrite) {
        loadWriteBuffer();
    }
}

bool requireReadRequest() {
    if(!canLockedRead) {
        error("Not locked for read.");
        return false;
    }
    if(readRequestSent) {
        return true;
    }
    twai_message_t canMsg = {};
    twaiMsg(&canMsg, canLockedForCmd->id);
    esp_err_t res = twai_transmit(&canMsg, pdMS_TO_TICKS(1000));
    if (res != ESP_OK) {
        errorUnlock("Failed sending CAN request for %s: %d", canLockedForCmd->attr->name, res);
        return false;
    }
    readRequestSent = true;
    return true;
}

void cleanReadBuffer() {
    bReadBuffer.totBytes = -1;
    bReadBuffer.rcvdBytes = 0;
    for (int i=0 ; i<MAX_RESULT_BUFF_LEN ; i++) {
        bReadBuffer.data[i] = 0;
    }
    for (int i=0 ; i<MAX_RESULT_FRAME_CNT ; i++) {
        bReadBuffer.rcvdFrames[i] = false;
    }
}

bool loadWriteBuffer() {
    if(!canLockedWrite) {
        error("Not locked for write.");
        return false;
    }
    bWriteBuffer.sentBytes = 0;
    for (int i=0 ; i<MAX_RESULT_FRAME_CNT ; i++) {
        bWriteBuffer.sentFrames[i] = false;
    }
    canLockedForCmd->attr->fromJson(doc, bWriteBuffer.data);
    bWriteBuffer.totBytes = canLockedForCmd->attr->dataSize();
    debugWriteBuffer("Loaded data to write", canLockedForCmd, &bWriteBuffer);
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
    return true;
}

void decodeToBMessage(const twai_message_t &msg) {
    bMessage.id = msg.identifier;
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

boolean isCtrlerToBsst() {
    return (
        bMessage.source == CAN_DRIVE_UNIT &&
        bMessage.target == CAN_BESST
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
        bMessage.op != CAN_MULTIFRAME_START ||
        bMessage.cmd != canLockedForCmd->attr->cmd ||
        bMessage.subCmd != canLockedForCmd->attr->subCmd
    ) {
        return false;
    }
    debugBMessage("MultiFrameStart received", &bMessage);
    if(!sendAck()) {
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
    if(bMessage.op == CAN_MULTIFRAME) {
        debugBMessage("MultiFrame received", &bMessage);
    } else if (bMessage.op == CAN_MULTIFRAME_END) {
        debugBMessage("MultiFrameEnd received", &bMessage);
    } else {
        return false;
    }
    if(!sendAck()) {
        return false;
    }
    saveMfBytes();
    debug("Total bytes saved/expected = %d/%d.",
        bReadBuffer.rcvdBytes, bReadBuffer.totBytes);
    return true;
}

bool sendAck() {
    if(!canLockedRead) {
        return false;
    }
    twai_message_t canMsg = {};
    twaiMsg(&canMsg, canLockedForCmd->ackId);
    esp_err_t res = twai_transmit(&canMsg, pdMS_TO_TICKS(1000));
    if (res != ESP_OK) {
        errorUnlock("Failed sending ACK to CAN: %d", res);
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
    doc["field"] = canLockedForCmd->attr->name;
    doc["cmd"] = canLockedForCmd->attr->cmd;
    doc["subCmd"] = canLockedForCmd->attr->subCmd;
    canLockedForCmd->attr->toJson(doc, bReadBuffer.data);
    serializeJson(doc, Serial);
    Serial.println();
}

bool requireStartMf() {
    if(!canLockedWrite) {
        return false;
    }

    // TODO
    return true;
}

bool canReceiveAck() {
    if(!canLockedWrite) {
        return false;
    }
    // TODO
    // if it's a const int CAN_ERROR_ACK = 0x03; then stop, error and unlock
    return true;
}

bool isCompleteOrWriteMf() {
    if(!canLockedWrite) {
        return false;
    }
    // TODO
    // return true if write is complete (all frames/bytes have been written)
    //    in this case all frames should have been also acknowledged already
    // otherwise write next frame and return false
    // in case of error do error and unlock
    return false;
}

void twaiMsg(
    twai_message_t *canMsg,
    uint32_t id,
    uint8_t dlc,
    uint8_t data[]
) { 
    canMsg->identifier = id;
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
    readRequestSent = false;
    canLockedWrite = false;
    canLockedForCmd = nullptr;
    canLockedAt = 0;
    debug("System unlocked.");
}
