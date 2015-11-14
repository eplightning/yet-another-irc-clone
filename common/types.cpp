#include <common/types.h>

#include <exception>

YAIC_NAMESPACE

Exception::Exception() :
    m_message()
{

}

Exception::Exception(String message) :
    m_message(message)
{

}

Exception::~Exception() throw()
{

}

const char *Exception::what() const throw()
{
    return m_message.c_str();
}

END_NAMESPACE
