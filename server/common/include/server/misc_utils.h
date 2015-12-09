#pragma once

#include <common/types.h>

#define EYAICDISCONNECT 100000
#define EYAICINVPACKET  100001

YAIC_NAMESPACE

class MiscUtils {
public:
    static void blockSignals();
    static void unblockSignals();
};

END_NAMESPACE
