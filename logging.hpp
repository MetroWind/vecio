#pragma once
#define DEBUG_LOGGING

#ifdef DEBUG_LOGGING
#include <iostream>

#endif

#include <string>

template <class T>
void log(const T& msg)
{
#ifdef DEBUG_LOGGING
    std::cerr << msg << std::endl;
#endif
}

template <class T, class... Args>
void log(const T& msg, Args... rest)
{
#ifdef DEBUG_LOGGING
    std::cerr << msg;
    log(rest...);
#endif
}

template <class t>
void logIoVar(const std::string& name, const T& var)
{
#ifdef DEBUG_LOGGING
    log(name, " is ", var, " (", sizeof(var), "bytes).");
#endif
}
