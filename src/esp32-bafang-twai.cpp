#include <Arduino.h>
#include <ArduinoJson.h>
#include "driver/twai.h"
#include "types.h"
#include "prototypes.h"
#include "util.h"
#include "logging.h"

#define TX_PIN GPIO_NUM_5
#define RX_PIN GPIO_NUM_4

const int MAX_SERIAL_BUFF_LEN = 512;
const unsigned long MAX_CAN_LOCK_MILLIS = 2000;

char serialBuff[MAX_SERIAL_BUFF_LEN];
bool canLockedRead = false;
bool canLockedWrite = false;
CanCommand *canLockedForCmd = nullptr;
unsigned long canLockedAt = 0;
BafangReadBuffer bReadBuffer{};
BafangWriteBuffer bWriteBuffer{};
BafangMessage bMessage{};

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
        && lockForSupportedCmd()
    ) {
        prepareLockedCmd();
    }
    if(canLockedRead
        && canReceiveMsg()
        && decodedMf()
        && completedMf()
    ) {
        returnBResult();
    }
    if(canLockedWrite) {
        //returnBResult();
    }
    if(canLockedForTooLong()) {
        errorUnlock("Timeout waiting multiframes.");
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
        errorUnlock("Error deserializing: %s", jsonParseError.c_str());
        return false;
    }
    return true;
}

bool lockForSupportedCmd() {
    if(canLockedRead || canLockedWrite) {
        return false;
    }
    CanCommand canCmd;
    for(int i=0 ; i<CAN_SUPPORTED_COMMANDS_COUNT ; i++) {
        canCmd = CAN_SUPPORTED_COMMANDS[i];
        if(!canLockByCmd(canCmd)) {
            continue;
        }
        // takes 400ms for some reason
        //debug("Locked for request: %s.", serialBuff);
        return true;
    }
    errorUnlock("Unrecognized request: %s.", serialBuff);
    return false;
}

bool canLockByCmd(CanCommand canCmd) {
    if(canLockedRead || canLockedWrite) {
        return false;
    }
    if(!doc["type"] || strcmp("req", doc["type"]) != 0) {
        return false;
    }
    if(!doc["field"] || strcmp(canCmd.attr->name, doc["field"]) != 0) {
        return false;
    }
    if(!doc["action"] || strcmp(canCmd.action, doc["action"]) != 0) {
        return false;
    }
    if(strcmp(canCmd.action, "read")) {
        canLockedRead = true;
        canLockedAt = millis();
        canLockedForCmd = &canCmd;
        return true;
    }
    if(strcmp(canCmd.action, "write")) {
        canLockedWrite = true;
        canLockedAt = millis();
        canLockedForCmd = &canCmd;
        return true;
    }
    return false;
}

void prepareLockedCmd() {
    if(canLockedRead) {
        canSendReadReq()
        && cleanReadBuffer();
    }
    if(canLockedWrite) {
        //loadWriteBuffer()
        //&& canSendMfStart();
    }
}

bool canSendReadReq() {
    if(!canLockedRead) {
        error("Not locked for read.");
        return false;
    }
    twai_message_t tx = {};
    tx.identifier = canLockedForCmd->id;
    tx.extd = 1;
    tx.rtr = 0;
    tx.data_length_code = 0;
    esp_err_t res = twai_transmit(&tx, pdMS_TO_TICKS(1000));
    if (res != ESP_OK) {
        errorUnlock("Failed sending CAN request for %s: %d", canLockedForCmd->attr->name, res);
        return false;
    }
    return true;
}

bool cleanReadBuffer() {
    if(!canLockedRead) {
        error("Not locked for read.");
        return false;
    }
    bReadBuffer.attr = NULL;
    bReadBuffer.totBytes = -1;
    bReadBuffer.rcvdBytes = 0;
    for (int i=0 ; i<MAX_RESULT_BUFF_LEN ; i++) {
        bReadBuffer.data[i] = 0;
    }
    for (int i=0 ; i<MAX_RESULT_FRAME_CNT ; i++) {
        bReadBuffer.rcvdFrames[i] = false;
    }
    return true;
}

bool loadWriteBuffer() {
    if(!canLockedWrite) {
        error("Not locked for write.");
        return false;
    }
    bWriteBuffer.id = canLockedForCmd->id;
    bWriteBuffer.attr = canLockedForCmd->attr;
    //TODO
}

// void initWriteBuffer() {
//     bWriteBuffer.id = -1;
//     bWriteBuffer.attr = NULL;
//     bWriteBuffer.totBytes = -1;
//     bWriteBuffer.sentBytes = 0;
//     for (int i=0 ; i<MAX_RESULT_FRAME_CNT ; i++) {
//         bWriteBuffer.sentFrames[i] = false;
//     }
//     for (int i=0 ; i<MAX_RESULT_BUFF_LEN ; i++) {
//         bWriteBuffer.data[i] = 0;
//     }
// }

bool canReceiveMsg() {
    if(!canLockedRead) {
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
    bReadBuffer.attr = attr;
    bReadBuffer.totBytes = bMessage.data[0];
    if(!sendAck()) {
        return false;
    }
    debug("Total bytes expected = %d", bMessage.data[0]);
    return true;
}

bool decodedMfFrame() {
    if(!canLockedRead) {
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
    debug("Total bytes saved/expected = %d/%d.", bReadBuffer.rcvdBytes, bReadBuffer.totBytes);
    return true;
}

bool sendAck() {
    if(!canLockedRead) {
        return false;
    }
    twai_message_t tx = {};
    tx.identifier = bafangCanAckId(*bReadBuffer.attr);
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

bool completedMf() {
    return (canLockedRead &&
        bReadBuffer.attr != NULL &&
        bReadBuffer.totBytes > 0 &&
        bReadBuffer.rcvdBytes == bReadBuffer.totBytes
    );
}

void returnBResult() {
    debugBResult("Returning", bReadBuffer);
    doc.clear();
    doc["type"] = "result";
    doc["field"] = bReadBuffer.attr->name;
    doc["cmd"] = bReadBuffer.attr->cmd;
    doc["subCmd"] = bReadBuffer.attr->subCmd;
    decodeCanData();
    serializeJson(doc, Serial);
    Serial.println();
    canUnlock();
}

void decodeCanData() {
    if(strcmp(bReadBuffer.attr->name, CAN_PARAM0.name) == 0) {
        decodeCanParam0();
    }
}

void decodeCanParam0() {
    doc["data"]["speedLim"] = bReadBuffer.data[0];
    for (int i = 0; i < 9; i++) {
        doc["data"]["accel"][i] = bReadBuffer.data[1 + i];
    }
    for (int i = 0; i < 9; i++) {
        int offset = 10 + i * 2;
        doc["data"]["assist"][i] =
            (uint16_t)bReadBuffer.data[offset] |
            ((uint16_t)bReadBuffer.data[offset + 1] << 8);
    }
    doc["data"]["assistUpLim"] =
        (uint16_t)bReadBuffer.data[28] |
        ((uint16_t)bReadBuffer.data[29] << 8);
    for (int i = 30; i <= 62; i++) {
        doc["data"]["unknown"][i-30] = bReadBuffer.data[i];
    }
    doc["data"]["checksum"] = bReadBuffer.data[63];
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
    canLockedWrite = false;
    canLockedForCmd = nullptr;
    canLockedAt = 0;
}
