#include <server/logger.h>

#include <common/types.h>

#include <string.h>
#include <time.h>
#include <errno.h>

YAIC_NAMESPACE

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

Logger::~Logger()
{

}

void Logger::error(const String &str)
{
    int err = errno;

    this->print(Line::Start);
    this->print(str);

    if (err && err != EAGAIN && err != EWOULDBLOCK) {
        char buf[256];

        buf[0] = ':';
        buf[1] = ' ';

        if (strerror_r(err, buf + 2, sizeof(buf) - 2) != -1)
            this->print(buf);
    }

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
