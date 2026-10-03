#include <Arduino.h>
#include "driver/twai.h"
#include <ArduinoJson.h>
#include "logging.h"

#define TX_PIN GPIO_NUM_5
#define RX_PIN GPIO_NUM_4

const int MAX_BUFF_LEN = 512;
const int MAX_RESULT_BUFF_LEN = 128;
const unsigned long MAX_CAN_LOCK_MILLIS = 2000;

struct BafangMessage {
    uint32_t id;
    uint8_t source;
    uint8_t target;
    uint8_t op;
    uint8_t cmd;
    uint8_t subCmd;
    uint8_t dlc;
    uint8_t data[8];
};

struct BafangResult {
    int totBytes;
    int rcvdBytes;
    uint8_t data[MAX_RESULT_BUFF_LEN];
};

char buff[MAX_BUFF_LEN];
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
    if(readSerialLine()) {
        if(canLocked) {
            warn("Already locked, ignoring request.");
        } else if(parseLine()) {
            if(lockIfReqFor("param0") || lockIfReqFor("param1")) {
                initBResult();
            } else {
                warn("Request or field not recognized: %s.", buff);
                return;
            }
        };
    }
    if(canReceiveMsg()) {
        if (decodeMfParam0() && mfComplete()) {
            returnParam0();
        } else
        if (decodeMfParam1() && mfComplete()) {
            returnParam1();
        }
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
      buff[idx] = '\0';
      idx = 0;
      return true;
    }
    if(idx < MAX_BUFF_LEN-1) {
      buff[idx++] = c;
    }
  }
  return false;
}

bool parseLine() {
    DeserializationError jsonParseError = deserializeJson(doc, buff);
    if (jsonParseError) {
        errorUnlock("Error deserializing: %s", jsonParseError.c_str());
        return false;
    }
    return true;
}

bool lockIfReqFor(const char* param) {
    if(!lockForReq(param)) {
        return false;
    }
    twai_message_t tx = {};
    tx.identifier = 0x85116010;
    tx.extd = 1;
    tx.rtr = 0;
    tx.data_length_code = 0;
    esp_err_t res = twai_transmit(&tx, pdMS_TO_TICKS(1000));
    if (res != ESP_OK) {
        errorUnlock("Failed sending CAN request for %s: %d", param, res);
        return false;
    }
    return true;
}

bool lockForReq(const char* param) {
    if(canLocked) {
        return false;
    }
    if(!doc["type"] || strcmp("req", doc["type"]) != 0) {
        return false;
    }
    if(!doc["field"] || strcmp(param, doc["field"]) != 0) {
        return false;
    }
    canLock();
    return true;
}

void initBResult() {
    bResult.totBytes = -1;
    bResult.rcvdBytes = 0;
    for (int i=0 ; i<MAX_RESULT_BUFF_LEN ; i++) {
        bResult.data[i] = NULL;
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
    return (bMessage.source == 0x02 && bMessage.target == 0x05);
}

bool decodeMfParam0() {
    if(!canLocked) {
        return false;
    }
    return (
        decodeMfStart(0x60, 0x10) ||
        decodeMfFrame()
    );
}

bool decodeMfStart(uint8_t expectedCmd, uint8_t expectedSubCmd) {
    if(!canLocked) {
        return false;
    }
    if(bMessage.op != 0x04 || bMessage.cmd != expectedCmd || bMessage.subCmd != expectedSubCmd) {
      return false;
    }
    if(!sendAck()) {
        return false;
    }
    int totBytesExpected = bMessage.data[0];
    bResult.totBytes = totBytesExpected;
    debugBMessage("MultiFrameStart");
    debug("Total bytes expected = %d", totBytesExpected);
    return true;
}

bool decodeMfFrame() {
    if(!canLocked) {
        return false;
    }
    if(bMessage.op != 0x05 && bMessage.op != 0x06) {
      return false;
    }
    if(!sendAck()) {
        return false;
    }
    saveMfBytes();
    debugBMessage("MultiFrame");
    debug("Total bytes saved/expected = %d/%d.", bResult.rcvdBytes, bResult.totBytes);
    return true;
}

bool sendAck() {
    if(!canLocked) {
        return false;
    }
    twai_message_t tx = {};
    tx.identifier = 0x85126010;
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
    int frmIdx = bMessage.subCmd;
    for (int i=0 ; i<bMessage.dlc ; i++) {
        bResult.data[frmIdx*8 + i] = bMessage.data[i];
    }
    bResult.rcvdBytes += bMessage.dlc;
}

bool mfComplete() {
    return (
        bResult.totBytes > 0 &&
        bResult.rcvdBytes == bResult.totBytes
    );
}

void returnParam0() {
    debug("Returning param0.");
    canUnlock();
}

bool canLockedForTooLong() {
    return canLocked && 
        (millis() - canLockedAt >= MAX_CAN_LOCK_MILLIS);
}

bool decodeMfParam1() {
    return false;
}

void returnParam1() {
    canUnlock();
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
    log(LOG_ERROR, format, args);
    va_end(args);
    canUnlock();
}

void debugBMessage(char* text) {
    char data[50];
    for (int i = 0; i < bMessage.dlc; i++) {
        sprintf(data+3*i, "%02X ", bMessage.data[i]);
    }
    debug("%s: ID=%08lX SRC=%02X DST=%02X DLC=%d OP=%02X CMD=%02X SUB=%02X DATA=%s",
        text, bMessage.id, bMessage.source, bMessage.target, bMessage.dlc,
        bMessage.op, bMessage.cmd, bMessage.subCmd, data
    );
}

