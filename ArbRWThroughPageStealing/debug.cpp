#include "debug.hpp"

void debug::printf_d(LogLevel level, const char* format, ...)
{

    if ((unsigned int)level >= DEBUG_LOG_LEVEL)
    {
        printf("[%s] ", debug::LogLevelString[(unsigned int) level]);
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format, args);
        printf("%s", buffer);
        va_end(args);
    }
    if (level == debug::LogLevel::FATAL)
    {
        throw std::invalid_argument("FATAL ERROR CHANCE\n");
        exit(-1);

    }
}

const char* debug::LogLevelString[] =
{
    "LOGG",
    "VRBS"
    "WARN",
    "ERRR",
    "FATL"
};