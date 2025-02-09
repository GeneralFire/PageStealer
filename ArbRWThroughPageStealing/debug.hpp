#pragma once
#include <stdio.h>
#include <cstdarg>
#include <exception>
#include <stdexcept>

#define DEBUG_LOG_LEVEL 1 // 0 > 1 > 2

class debug
{
	static const char* LogLevelString[];

public:

	enum class LogLevel
	{
		LOG = 0,
		VERBOSE,
		WARN,
		ERR,
		FATAL
	};

	static void printf_d(LogLevel Level, const char* format, ...);
};
