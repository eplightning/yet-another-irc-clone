#include <server/log.h>

#include <common/types.h>

#include <time.h>

YAIC_NAMESPACE

Log &operator<<(Log &log, const String &str)
{
    log.print(str);
    return log;
}

Log &operator<<(Log &log, Log::Line marker)
{
    log.print(marker);
    return log;
}

Log &operator<<(Log &log, long long integer)
{
    log.print(integer);
    return log;
}

Log &operator<<(Log &log, unsigned long long integer)
{
    log.print(integer);
    return log;
}

Log &operator<<(Log &log, int integer)
{
    log.print(static_cast<long long>(integer));
    return log;
}

Log &operator<<(Log &log, uint integer)
{
    log.print(static_cast<unsigned long long>(integer));
    return log;
}

Log &operator<<(Log &log, double floating)
{
    log.print(floating);
    return log;
}

Log::~Log()
{

}

String Log::date() const
{
    time_t time = ::time(NULL);
    struct tm local;

    localtime_r(&time, &local);

    char buffer[128];

    strftime(buffer, sizeof(buffer), "%c", &local);

    return buffer;
}

END_NAMESPACE
