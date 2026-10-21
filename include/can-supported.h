#pragma once

#include "can-const.h"
#include "can-util.h"
#include "can-command.h"

#include "can-codecs/hardware.h"
#include "can-codecs/software.h"
#include "can-codecs/param0.h"

const CanCommand CAN_SUPPORTED[] = {
    { .action = "read",  .codec = Hardware::instance() },
    { .action = "read",  .codec = Software::instance() },
    { .action = "read",  .codec = Param0::instance()   },
    { .action = "write", .codec = Param0::instance()   }
};

const int CAN_SUPPORTED_COUNT = sizeof(CAN_SUPPORTED)
    / sizeof(CAN_SUPPORTED[0]);
