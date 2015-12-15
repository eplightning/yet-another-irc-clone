#pragma once

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_manager.h>
#include <server/logger.h>
#include <server/syseventloop.h>

YAIC_NAMESPACE

// nie chce robić .cpp więc daje #define zamiast const static int
#define SLAVE_APP_SOURCE_USER  1
#define SLAVE_APP_SOURCE_MASTER 2
#define SLAVE_APP_SOURCE_SLAVE 3

END_NAMESPACE
