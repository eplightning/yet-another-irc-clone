#include <server/logger/stdout.h>

#include <common/types.h>
#include <server/misc_utils.h>
#include <server/logger.h>

#include <iostream>

#include <errno.h>

YAIC_NAMESPACE

void LoggerStdout::print(const String &str)
{
    std::cout << str;
}

void LoggerStdout::print(Logger::Line marker)
{
    int err;

    switch (marker) {
    case Logger::Line::End:
        if (m_errno != 0) {
            String error = MiscUtils::systemError(m_errno);

            if (!error.empty())
                std::cout << ": " << error;
        }

        std::cout << std::endl;
        m_errno = 0;
        m_lock.unlock();
        break;
    case Logger::Line::StartError:
        err = errno;
        m_lock.lock();
        std::cout << "[" << date() << "] ";
        m_errno = err;
        break;
    case Logger::Line::Start:
        m_lock.lock();
        std::cout << "[" << date() << "] ";
        break;
    }
}

void LoggerStdout::print(unsigned long long integer)
{
    std::cout << integer;
}

void LoggerStdout::print(long long integer)
{
    std::cout << integer;
}

void LoggerStdout::print(double floating)
{
    std::cout << floating;
}

END_NAMESPACE
