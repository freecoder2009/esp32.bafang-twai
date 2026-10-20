#pragma once

#include "can-codec.h"

struct CanCommand {
    const CanCodec* attr;
    const char* action;
    const int op;
    uint32_t canId;
    uint32_t ackId;
};
