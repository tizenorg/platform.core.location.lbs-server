/*
 *  Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @file    common/Logger.h
 * @brief   Simple logger
 */

#pragma once
#ifndef __FL_LOGGER_H__
#define __FL_LOGGER_H__

#define LOG_LEVEL_OFF       0
#define LOG_LEVEL_FATAL     1
#define LOG_LEVEL_ERROR     2
#define LOG_LEVEL_WARNING   3
#define LOG_LEVEL_INFO      4
#define LOG_LEVEL_TRACE     5
#define LOG_LEVEL_DETAIL    6

#include <stdio.h>
#include <iostream> // NOTE: This is temporary solution
#include <string>
#include <cstdint>
#include "DebugBreak.h"

#if !defined(LOG_THRESHOLD)
	// default, compilation-mode dependent level
	#if defined(DEBUG) || defined(_DEBUG)
		// debug
		#define LOG_THRESHOLD   LOG_LEVEL_WARNING
	#elif !defined(NDEBUG)
		// internal release
		#define LOG_THRESHOLD   LOG_LEVEL_ERROR
	#else
		// final release
		#define LOG_THRESHOLD   LOG_LEVEL_OFF
	#endif
#endif

namespace fl {
namespace logger {

enum LogLevelSymbol : char {
	LS_FATAL   = 'F',
	LS_ERROR   = 'E',
	LS_WARNING = 'W',
	LS_INFO    = 'I',
	LS_TRACE   = 'T',
	LS_DEBUG   = 'D',
};

enum LogLimits {
	MAX_MESSAGE_LENGTH = 256
};

extern char __buffer[MAX_MESSAGE_LENGTH + 1];

inline void log(LogLevelSymbol levelSymbol, const std::string& message, const char* file, unsigned line)
{
	std::cerr << ":: " << char(levelSymbol) << " :: " << file << " : " << line << " -:\t"
	          << message << std::endl;
}

} // fl::logger
} // fl

#define LOGM(LEVEL, MESSAGE)                                                                       \
	do {                                                                                           \
		fl::logger::log(LEVEL, MESSAGE, __FILE__, __LINE__);                                       \
	} while (false)

#define LOGFM(LEVEL, format, ...)                                                                  \
	do {                                                                                           \
		snprintf(fl::logger::__buffer, fl::logger::MAX_MESSAGE_LENGTH, format, ##__VA_ARGS__);     \
		fl::logger::__buffer[fl::logger::MAX_MESSAGE_LENGTH] = 0;                                  \
		fl::logger::log(LEVEL, fl::logger::__buffer, __FILE__, __LINE__);                          \
	} while (false)

#define LOGD(MESSAGE) LOGM(fl::logger::LS_DEBUG,   MESSAGE)
#define LOGT(MESSAGE) LOGM(fl::logger::LS_TRACE,   MESSAGE)
#define LOGI(MESSAGE) LOGM(fl::logger::LS_INFO,    MESSAGE)
#define LOGW(MESSAGE) LOGM(fl::logger::LS_WARNING, MESSAGE)
#define LOGE(MESSAGE) LOGM(fl::logger::LS_ERROR,   MESSAGE)
#define LOGF(MESSAGE) LOGM(fl::logger::LS_FATAL,   MESSAGE)

#if defined(_MSC_VER)
// in MS C++ compiler the __FUNCTION__ is a macro string and can be
// concatenated with other strings at the compilation time
	#define FUNCTION_PREFIX(format)          __FUNCTION__ format
	#define FUNCTION_THIS_PREFIX(format)     __FUNCTION__ "[%p]" format, this
#else
// in Xcode and GCC the __FUNCTION__ is a constant, and cannot be concatenated
// with other strings at the compilation time
	#define FUNCTION_PREFIX(format)          "%s" format, __FUNCTION__
	#define FUNCTION_THIS_PREFIX(format)     "%s[%p]" format, __FUNCTION__, this
#endif
#define LOG_INDENT_PREFIX                   "> "
#define LOG_UNINDENT_PREFIX                 "< "
#define ENTER_FUNCTION_PREFIX(format)       LOG_INDENT_PREFIX   FUNCTION_PREFIX(format)
#define LEAVE_FUNCTION_PREFIX(format)       LOG_UNINDENT_PREFIX FUNCTION_PREFIX(format)
#define ENTER_FUNCTION_THIS_PREFIX(format)  LOG_INDENT_PREFIX   FUNCTION_THIS_PREFIX(format)
#define LEAVE_FUNCTION_THIS_PREFIX(format)  LOG_UNINDENT_PREFIX FUNCTION_THIS_PREFIX(format)

#if (LOG_THRESHOLD >= LOG_LEVEL_DETAIL)
	#define LOG_DETAIL(format, ...)         LOGFM(fl::logger::LS_DEBUG, format, ##__VA_ARGS__)
#else
	#define LOG_DETAIL(format, ...)         ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_TRACE)
	#define LOG_TRACE(format, ...)          LOGFM(fl::logger::LS_TRACE, format, ##__VA_ARGS__)
#else
	#define LOG_TRACE(format, ...)          ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_INFO)
	#define LOG_INFO(format, ...)           LOGFM(fl::logger::LS_INFO, format, ##__VA_ARGS__)
#else
	#define LOG_INFO(format, ...)           ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_WARNING)
	#define LOG_WARNING(format, ...)        LOGFM(fl::logger::LS_WARNING, format, ##__VA_ARGS__)
#else
	#define LOG_WARNING(format, ...)        ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_ERROR)
	#define LOG_ERROR(format, ...)          LOGFM(fl::logger::LS_ERROR, format, ##__VA_ARGS__)
#else
	#define LOG_ERROR(format, ...)          ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_FATAL)
	#define LOG_FATAL(format, ...)          do {DEBUGBREAK(); LOGFM(fl::logger::LS_FATAL, format, ##__VA_ARGS__);} while (false)
#else
	#define LOG_FATAL(format, ...)          DEBUGBREAK()
#endif

#endif // __FL_LOGGER_H__
