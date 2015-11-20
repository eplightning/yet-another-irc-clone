#pragma once

#include <common/types.h>

YAIC_NAMESPACE

class MiscUtils {
public:
    static void blockSignals();
    static void unblockSignals();
};

END_NAMESPACE
