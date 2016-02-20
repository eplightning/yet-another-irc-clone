#include <server/logger.h>

#include <common/types.h>

#include <string.h>
#include <time.h>
#include <errno.h>

YAIC_NAMESPACE

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, const char *str)
{
    log->print(str);
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, const String &str)
{
    log->print(str);
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, Logger::Line marker)
{
    log->print(marker);
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, long long integer)
{
    log->print(integer);
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, unsigned long long integer)
{
    log->print(integer);
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, unsigned long integer)
{
    log->print(static_cast<unsigned long long>(integer));
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, long integer)
{
    log->print(static_cast<long long>(integer));
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, int integer)
{
    log->print(static_cast<long long>(integer));
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, uint integer)
{
    log->print(static_cast<unsigned long long>(integer));
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, double floating)
{
    log->print(floating);
    return log;
}

UniquePtr<Logger> &operator<<(UniquePtr<Logger> &log, bool flag)
{
    log->printBool(flag);
    return log;
}

Logger::~Logger()
{

}

void Logger::error(const String &str)
{
    this->print(Line::StartError);
    this->print(str);
    this->print(Line::End);
}

void Logger::message(const String &str)
{
    this->print(Line::Start);
    this->print(str);
    this->print(Line::End);
}

String Logger::date() const
{
    time_t time = ::time(NULL);
    struct tm local;

    localtime_r(&time, &local);

    char buffer[128];

    strftime(buffer, sizeof(buffer), "%c", &local);

    return buffer;
}

END_NAMESPACE
