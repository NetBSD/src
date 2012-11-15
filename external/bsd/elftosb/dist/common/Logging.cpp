/*
 * File:	Logging.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "Logging.h"
#include <stdarg.h>
#include <stdio.h>
#include "smart_ptr.h"

// init global logger to null
Logger * Log::s_logger = NULL;

void Logger::log(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	log(m_level, fmt, args);
	va_end(args);
}

void Logger::log(log_level_t level, const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	log(level, fmt, args);
	va_end(args);
}

void Logger::log(const char * fmt, va_list args)
{
	log(m_level, fmt, args);
}

//! Allocates a temporary 1KB buffer which is used to hold the
//! formatted string.
void Logger::log(log_level_t level, const char * fmt, va_list args)
{
	smart_array_ptr<char> buffer = new char[1024];
	vsprintf(buffer, fmt, args);
	if (level <= m_filter)
	{
		_log(buffer);
	}
}

void Log::log(const char * fmt, ...)
{
	if (s_logger)
	{
		va_list args;
		va_start(args, fmt);
		s_logger->log(fmt, args);
		va_end(args);
	}
}

void Log::log(const std::string & msg)
{
	if (s_logger)
	{
		s_logger->log(msg);
	}
}

void Log::log(Logger::log_level_t level, const char * fmt, ...)
{
	if (s_logger)
	{
		va_list args;
		va_start(args, fmt);
		s_logger->log(level, fmt, args);
		va_end(args);
	}
}

void Log::log(Logger::log_level_t level, const std::string & msg)
{
	if (s_logger)
	{
		s_logger->log(level, msg);
	}
}

void StdoutLogger::_log(const char * msg)
{
	printf(msg);
}

