#pragma once

#include "can-const.h"
#include "can-util.h"
#include "can-command.h"

#include "can-codecs/software.h"
#include "can-codecs/param0.h"

const CanCommand CAN_SUPPORTED[] = {
    { .action = "read",  .attr = Software::instance() },
    { .action = "read",  .attr = Param0::instance()   },
    { .action = "write", .attr = Param0::instance()   }
};

const int CAN_SUPPORTED_COUNT = sizeof(CAN_SUPPORTED)
    / sizeof(CAN_SUPPORTED[0]);
