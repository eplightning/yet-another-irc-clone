#pragma once

#include <common/types.h>

YAIC_NAMESPACE

class Logger {
public:
    enum class Line {
        Start = 0,
        End = 1,
        StartError = 2
    };

    virtual ~Logger();

    virtual void print(const String &str) = 0;
    virtual void print(Line marker) = 0;
    virtual void print(long long integer) = 0;
    virtual void print(unsigned long long integer) = 0;
    virtual void print(double floating) = 0;
    virtual void printBool(bool flag) = 0;

    void error(const String &str);
    void message(const String &str);

protected:
    String date() const;
};

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, const String &str);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, const char *str);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, Logger::Line marker);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, long long integer);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, long integer);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, unsigned long long integer);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, unsigned long integer);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, int integer);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, uint integer);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, double floating);
UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, bool flag);

END_NAMESPACE
