#pragma once

#include <common/types.h>

#define EYAICDISCONNECT 100000
#define EYAICINVPACKET  100001

YAIC_NAMESPACE

/**
 * @brief Różne pomocnicze funkcje
 */
class MiscUtils {
public:
    static void blockSignals();
    static void unblockSignals();

    static String systemError(int num);
};

END_NAMESPACE
