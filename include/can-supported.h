#pragma once

#include "can-const.h"
#include "can-util.h"
#include "can-command.h"

#include "can-codecs/software.h"
#include "can-codecs/param0.h"

using namespace BafangCan;

const CanCommand CAN_SUPPORTED[] = {
    {
        .attr      = Software::instance(),
        .action    = "read",
        .op        = CAN_OP_READ,
        .canId     = canGetReadId(Software::instance()),
        .ackId     = canGetReadAckId(Software::instance())
    },
    {
        .attr      = Param0::instance(),
        .action    = "read",
        .op        = CAN_OP_READ,
        .canId     = canGetReadId(Param0::instance()),
        .ackId     = canGetReadAckId(Param0::instance())
    },
    {
        .attr      = Param0::instance(),
        .action    = "write",
        .op        = CAN_OP_WRITE,
        .canId     = canGetWriteStartId(Param0::instance()),
        .ackId     = canGetWriteAckId(Param0::instance())
    }
};

const int CAN_SUPPORTED_COUNT = sizeof(CAN_SUPPORTED)
    / sizeof(CAN_SUPPORTED[0]);