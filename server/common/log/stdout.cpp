#include <server/log/stdout.h>

#include <common/types.h>
#include <server/log.h>

#include <iostream>

YAIC_NAMESPACE

void LogStdout::print(const String &str)
{
    std::cout << str;
}

void LogStdout::print(Log::Line marker)
{
    switch (marker) {
    case Log::Line::End:
        std::cout << std::endl;
        break;
    case Log::Line::Start:
        std::cout << "[" << date() << "] ";
        break;
    }
}

void LogStdout::print(unsigned long long integer)
{
    std::cout << integer;
}

void LogStdout::print(long long integer)
{
    std::cout << integer;
}

void LogStdout::print(double floating)
{
    std::cout << floating;
}

END_NAMESPACE
