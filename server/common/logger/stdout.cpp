#include <server/logger/stdout.h>

#include <common/types.h>
#include <server/logger.h>

#include <iostream>

YAIC_NAMESPACE

void LoggerStdout::print(const String &str)
{
    std::cout << str;
}

void LoggerStdout::print(Logger::Line marker)
{
    switch (marker) {
    case Logger::Line::End:
        std::cout << std::endl;
        m_lock.unlock();
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
