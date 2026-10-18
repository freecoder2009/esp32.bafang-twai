#pragma once

#include <ArduinoJson.h>
#include "types.h"
#include "prototypes.h"

extern const int LOG_LEVEL;

const int LOG_DEBUG = 1;
const int LOG_INFO = 2;
const int LOG_WARN = 3;
const int LOG_ERROR = 4;

char logBuff[512];
JsonDocument logDoc;

void logToSerial(const int level) {
    logDoc.clear();
    logDoc["type"] = "log";
    logDoc["time"] = millis();
    logDoc["level"] = level;
    logDoc["message"] = logBuff;
    serializeJson(logDoc, Serial);
    Serial.println();
}

void vlog(const int level, const char* format, va_list va) {
    vsprintf(logBuff, format, va);
    logToSerial(level);
}

void log(const int level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(level, format, args);
    va_end(args);
}

void debug(const char* format, ...) {
    if(LOG_LEVEL > LOG_DEBUG) {
        return;
    }
    va_list args;
    va_start(args, format);
    vlog(LOG_DEBUG, format, args);
    va_end(args);
}

void info(const char* format, ...) {
    if(LOG_LEVEL > LOG_INFO) {
        return;
    }
    va_list args;
    va_start(args, format);
    vlog(LOG_INFO, format, args);
    va_end(args);
}

void warn(const char* format, ...) {
    if(LOG_LEVEL > LOG_WARN) {
        return;
    }
    va_list args;
    va_start(args, format);
    vlog(LOG_WARN, format, args);
    va_end(args);
}

void error(const char* format, ...) {
    if(LOG_LEVEL > LOG_ERROR) {
        return;
    }
    va_list args;
    va_start(args, format);
    vlog(LOG_ERROR, format, args);
    va_end(args);
}

void debugBMessage(const char* text, const CanMessage *bMessage) {
    if(LOG_LEVEL > LOG_DEBUG) {
      return;
    }
    char data[bMessage->dlc*3 + 10];
    for (int i=0; i<bMessage->dlc; i++) {
        sprintf(data+3*i, "%02X ", bMessage->data[i]);
    }
    log(LOG_DEBUG, "%s: ID=%08lX SRC=%02X DST=%02X DLC=%d OP=%02X CMD=%02X SUB=%02X DATA=%s",
        text, bMessage->rwStartId, bMessage->source, bMessage->target, bMessage->dlc,
        bMessage->op, bMessage->cmd, bMessage->subCmd, data
    );
}

void debugReadBuffer(const char* text, const CanCommand *canLockedForCmd, const CanReadBuffer *bReadBuffer) {
    if(LOG_LEVEL > LOG_DEBUG) {
        return;
    }
    char data[bReadBuffer->totBytes*3 + 10];
    for (int i=0; i<bReadBuffer->totBytes; i++) {
        sprintf(data+3*i, "%02X ", bReadBuffer->data[i]);
    }
    log(LOG_DEBUG, "%s: FIELD=%s CMD=%02X SUB=%02X BYTES:%d DATA=%s",
        text, canLockedForCmd->attr->name, canLockedForCmd->attr->cmd, 
        canLockedForCmd->attr->subCmd, bReadBuffer->totBytes, data
    );
}

void debugWriteBuffer(const char* text, const CanCommand *canLockedForCmd, const CanWriteBuffer *bWriteBuffer) {
    if(LOG_LEVEL > LOG_DEBUG) {
        return;
    }
    char data[bWriteBuffer->totBytes*3 + 10];
    for (int i=0; i<bWriteBuffer->totBytes; i++) {
        sprintf(data+3*i, "%02X ", bWriteBuffer->data[i]);
    }
    log(LOG_DEBUG, "%s: FIELD=%s CMD=%02X SUB=%02X BYTES:%d DATA=%s",
        text, canLockedForCmd->attr->name, canLockedForCmd->attr->cmd, 
        canLockedForCmd->attr->subCmd, bWriteBuffer->totBytes, data
    );
}