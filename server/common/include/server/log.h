#pragma once

#include <common/types.h>

YAIC_NAMESPACE

class Log {
public:
    enum class Line {
        Start = 0,
        End = 1
    };

    virtual ~Log();

    virtual void print(const String &str) = 0;
    virtual void print(Line marker) = 0;
    virtual void print(long long integer) = 0;
    virtual void print(unsigned long long integer) = 0;
    virtual void print(double floating) = 0;

protected:
    String date() const;
};

Log &operator<<(Log &log, const String &str);
Log &operator<<(Log &log, Log::Line marker);
Log &operator<<(Log &log, long long integer);
Log &operator<<(Log &log, unsigned long long integer);
Log &operator<<(Log &log, int integer);
Log &operator<<(Log &log, uint integer);
Log &operator<<(Log &log, double floating);

END_NAMESPACE
