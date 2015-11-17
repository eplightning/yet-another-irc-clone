#pragma once

#include <common/types.h>
#include <server/log.h>

#include <iostream>

YAIC_NAMESPACE

class LogStdout : public Log {
public:
    void print(const String &str);
    void print(Line marker);
    void print(long long integer);
    void print(unsigned long long integer);
    void print(double floating);
};

END_NAMESPACE
