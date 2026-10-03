#include <ArduinoJson.h>

const int LOG_DEBUG = 1;
const int LOG_INFO = 2;
const int LOG_WARN = 3;
const int LOG_ERROR = 4;

const int LOG_LEVEL = LOG_DEBUG;
const int MAX_LOG_BUFF_LEN = 512;

char logBuff[MAX_LOG_BUFF_LEN];
JsonDocument logDoc;

void log(const int level, const char* format, va_list va) {
    if(LOG_LEVEL > level) {
      return;
    }
    vsprintf(logBuff, format, va);
    logDoc.clear();
    logDoc["type"] = "log";
    logDoc["time"] = millis();
    logDoc["level"] = level;
    logDoc["message"] = logBuff;
    serializeJson(logDoc, Serial);
    Serial.println();
}

void debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_DEBUG, format, args);
    va_end(args);
}

void info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_INFO, format, args);
    va_end(args);
}

void warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_WARN, format, args);
    va_end(args);
}

void error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_ERROR, format, args);
    va_end(args);
}
