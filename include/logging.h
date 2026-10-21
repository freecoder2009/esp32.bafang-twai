#pragma once

#include <ArduinoJson.h>
#include "types.h"
#include "prototypes.h"

extern const int LOG_LEVEL;

const int LOG_TRACE = 0;
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

void trace(const char* format, ...) {
    if(LOG_LEVEL > LOG_TRACE) {
        return;
    }
    va_list args;
    va_start(args, format);
    vlog(LOG_TRACE, format, args);
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

void traceCanMsg(const char* text, const twai_message_t *canMsg) {
    if(LOG_LEVEL > LOG_TRACE) {
        return;
    }
    int totBytes = canMsg->data_length_code;
    int buffSize = totBytes*3 + 1;
    char buff[buffSize];
    for (int i=0; i<totBytes; i++) {
        sprintf(buff+3*i, "%02X ", canMsg->data[i]);
    }
    buff[buffSize-1] = '\0';
    log(LOG_TRACE, "%s: ID=%08lX DLC=%d DATA=%s",
        text, canMsg->identifier, canMsg->data_length_code, buff
    );
}

void logBMessage(const int level, const char* text, const CanMessage *bMessage) {
    if(LOG_LEVEL > level) {
      return;
    }
    int totBytes = bMessage->dlc;
    int buffSize = totBytes*3 + 1;
    char buff[buffSize];
    for (int i=0; i<totBytes; i++) {
        sprintf(buff+3*i, "%02X ", bMessage->data[i]);
    }
    buff[buffSize-1] = '\0';
    log(level, "%s: ID=%08lX SRC=%02X DST=%02X DLC=%d OP=%02X CMD=%02X SUB=%02X DATA=%s",
        text, bMessage->canId, bMessage->source, bMessage->target, bMessage->dlc,
        bMessage->op, bMessage->cmd, bMessage->subCmd, buff
    );
}

void debugReadBuffer(const char* text, const CanCommand *canLockedForCmd, const CanReadBuffer *bReadBuffer) {
    if(LOG_LEVEL > LOG_DEBUG) {
        return;
    }
    int totBytes = bReadBuffer->totBytes;
    int buffSize = totBytes*3 + 1;
    char buff[buffSize];
    for (int i=0; i<totBytes; i++) {
        sprintf(buff+3*i, "%02X ", bReadBuffer->data[i]);
    }
    buff[buffSize-1] = '\0';
    log(LOG_DEBUG, "%s: FIELD=%s CMD=%02X SUB=%02X BYTES:%d DATA=%s",
        text, canLockedForCmd->codec->name, canLockedForCmd->codec->cmd, 
        canLockedForCmd->codec->subCmd, bReadBuffer->totBytes, buff
    );
}

void debugWriteBuffer(const char* text, const CanCommand *canLockedForCmd, const CanWriteBuffer *bWriteBuffer) {
    if(LOG_LEVEL > LOG_DEBUG) {
        return;
    }
    int totBytes = bWriteBuffer->totBytes;
    int buffSize = totBytes*3 + 1;
    char buff[buffSize];
    for (int i=0; i<totBytes; i++) {
        sprintf(buff+3*i, "%02X ", bWriteBuffer->data[i]);
    }
    buff[buffSize-1] = '\0';
    log(LOG_DEBUG, "%s: FIELD=%s CMD=%02X SUB=%02X BYTES:%d DATA=%s",
        text, canLockedForCmd->codec->name, canLockedForCmd->codec->cmd, 
        canLockedForCmd->codec->subCmd, bWriteBuffer->totBytes, buff
    );
}
