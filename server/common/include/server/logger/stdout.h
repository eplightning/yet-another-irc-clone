#pragma once

#include <common/types.h>
#include <server/logger.h>

#include <iostream>
#include <mutex>

YAIC_NAMESPACE

class LoggerStdout : public Logger {
public:
    LoggerStdout();

    void print(const String &str);
    void print(Line marker);
    void print(long long integer);
    void print(unsigned long long integer);
    void print(double floating);
    void print(bool flag);

protected:
    std::mutex m_lock;
    int m_errno;
};

END_NAMESPACE
