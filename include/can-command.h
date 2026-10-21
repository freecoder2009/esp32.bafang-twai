#pragma once

#include "can-codec.h"

struct CanCommand {
    const char* action;
    const CanCodec* codec;
};
