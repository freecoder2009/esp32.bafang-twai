#include <Arduino.h>
#include "driver/twai.h"
#include <ArduinoJson.h>
#include "util.h"
#include "logging.h"

#define TX_PIN GPIO_NUM_5
#define RX_PIN GPIO_NUM_4

const int MAX_SERIAL_BUFF_LEN = 512;
const unsigned long MAX_CAN_LOCK_MILLIS = 2000;

char serialBuff[MAX_SERIAL_BUFF_LEN];
bool canLocked = false;
unsigned long canLockedAt = 0;
BafangMessage bMessage{};
BafangResult bResult{};

JsonDocument doc;

void setup() {
    Serial.begin(1000000);
    while (!Serial);
    Serial.println();
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

void loop() {
    if(readSerialLine()
        && requireCanUnlocked()
        && parseSerialLine()
        && lockForSupportedCommand()
    ) {
        initBResult();
    }
    if(canReceiveMsg()
        && decodedMf()
        && completedMf()
    ) {
        returnBResult();
    }
    if(canLockedForTooLong()) {
        errorUnlock("Timeout waiting multiframes.");
    }
}

boolean readSerialLine() {
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
    if(canLocked) {
        warn("CAN is already locked.");
        return false;
    }
    return true;
}

bool parseSerialLine() {
    DeserializationError jsonParseError = deserializeJson(doc, serialBuff);
    if (jsonParseError) {
        errorUnlock("Error deserializing: %s", jsonParseError.c_str());
        return false;
    }
    return true;
}

bool lockForSupportedCommand() {
    CanCommand request;
    for(int i=0 ; i<CAN_SUPPORTED_COMMANDS_COUNT ; i++) {
        request = CAN_SUPPORTED_COMMANDS[i];
        if(!lockForReq(request)) {
            continue;
        }
        if(!canSendReq(request)) {
            return false;
        }
        // takes 400ms for some reason
        //debug("Locked for request: %s.", serialBuff);
        return true;
    }
    errorUnlock("Unrecognized request: %s.", serialBuff);
    return false;
}

bool lockForReq(CanCommand param) {
    if(canLocked) {
        return false;
    }
    if(!doc["type"] || strcmp("req", doc["type"]) != 0) {
        return false;
    }
    if(!doc["field"] || strcmp(param.attr->name, doc["field"]) != 0) {
        return false;
    }
    if(!doc["action"] || strcmp(param.action, doc["action"]) != 0) {
        return false;
    }
    canLock();
    return true;
}

bool canSendReq(CanCommand canCmd) {
    twai_message_t tx = {};
    tx.identifier = canCmd.id;
    tx.extd = 1;
    tx.rtr = 0;
    tx.data_length_code = 0;
    esp_err_t res = twai_transmit(&tx, pdMS_TO_TICKS(1000));
    if (res != ESP_OK) {
        errorUnlock("Failed sending CAN request for %s: %d", canCmd.attr->name, res);
        return false;
    }
    return true;
}

void initBResult() {
    bResult.attr = NULL;
    bResult.totBytes = -1;
    bResult.rcvdBytes = 0;
    for (int i=0 ; i<MAX_RESULT_BUFF_LEN ; i++) {
        bResult.data[i] = NULL;
    }
    for (int i=0 ; i<MAX_RESULT_FRAME_CNT ; i++) {
        bResult.rcvdFrames[i] = false;
    }
}

bool canReceiveMsg() {
    if(!canLocked) {
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

bool decodedMf() {
    if(!canLocked) {
        return false;
    }
    if(decodedMfStart() || decodedMfFrame()) {
        return true;
    }
    return false;
}

bool decodedMfStart() {
    if(!canLocked) {
        return false;
    }
    const CanAttr *attr;
    bool found = false;
    for(int i=0 ; i<CAN_SUPPORTED_COMMANDS_COUNT ; i++) {
        attr = CAN_SUPPORTED_COMMANDS[i].attr;
        if(
            bMessage.op == CAN_MULTIFRAME_START &&
            bMessage.cmd == attr->cmd &&
            bMessage.subCmd == attr->subCmd
        ) {
            found = true;
            break;
        }
    }
    if(!found) {
        return false;
    }
    debugBMessage("MultiFrameStart", bMessage);
    bResult.attr = attr;
    bResult.totBytes = bMessage.data[0];
    if(!sendAck()) {
        return false;
    }
    debug("Total bytes expected = %d", bMessage.data[0]);
    return true;
}

bool decodedMfFrame() {
    if(!canLocked) {
        return false;
    }
    if(
        bMessage.op != CAN_MULTIFRAME &&
        bMessage.op != CAN_MULTIFRAME_END
    ) {
      return false;
    }
    debugBMessage("MultiFrame", bMessage);
    saveMfBytes();
    if(!sendAck()) {
        return false;
    }
    debug("Total bytes saved/expected = %d/%d.", bResult.rcvdBytes, bResult.totBytes);
    return true;
}

bool sendAck() {
    if(!canLocked) {
        return false;
    }
    twai_message_t tx = {};
    tx.identifier = bafangCanAckId(*bResult.attr);
    tx.extd = 1;
    tx.rtr = 0;
    tx.data_length_code = 0;
    esp_err_t res = twai_transmit(&tx, pdMS_TO_TICKS(1000));
    if (res != ESP_OK) {
        errorUnlock("Failed sending ACK to CAN: %d", res);
        return false;
    }
    return true;
}

void saveMfBytes() {
    uint8_t frmIdx = bMessage.subCmd;
    if(bResult.rcvdFrames[frmIdx]) {
        warn("Duplicate frame #%d received from CAN, ignoring.", frmIdx);
        return;
    }
    bResult.rcvdFrames[frmIdx] = true;
    for (int i=0 ; i<bMessage.dlc ; i++) {
        bResult.data[frmIdx*8 + i] = bMessage.data[i];
    }
    bResult.rcvdBytes += bMessage.dlc;
}

bool completedMf() {
    return (
        bResult.attr != NULL &&
        bResult.totBytes > 0 &&
        bResult.rcvdBytes == bResult.totBytes
    );
}

void returnBResult() {
    debugBResult("Returning", bResult);
    canUnlock();
}

bool canLockedForTooLong() {
    return canLocked && 
        (millis() - canLockedAt >= MAX_CAN_LOCK_MILLIS);
}

void canLock() {
    canLocked = true;
    canLockedAt = millis();
}

void canUnlock() {
    canLocked = false;
    canLockedAt = 0;
}

void errorUnlock(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LOG_ERROR, format, args);
    va_end(args);
    canUnlock();
}
