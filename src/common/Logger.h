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

#include <iostream> // NOTE: This is temporary solution
#include <string>
#include <cstdint>
#include <stddef.h>
#include "Result.h"
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

enum class LevelSymbol : char {
	FATAL   = 'F',
	ERROR   = 'E',
	WARNING = 'W',
	INFO    = 'I',
	TRACE   = 'T',
	DEBUG   = 'D',
}; // fl::logger::LevelSymbol

enum class Level {
	OFF     = LOG_LEVEL_OFF,
	FATAL   = LOG_LEVEL_FATAL,
	ERROR   = LOG_LEVEL_ERROR,
	WARNING = LOG_LEVEL_WARNING,
	INFO    = LOG_LEVEL_INFO,
	TRACE   = LOG_LEVEL_TRACE,
	DETAIL  = LOG_LEVEL_DETAIL,
	// always the last
	COUNT
}; // fl::logger::Level

inline void print(LevelSymbol levelSymbol, const std::string& message, const char* file, unsigned line)
{
	std::cerr << ":: " << static_cast<char>(levelSymbol) << " :: " << file << " : " << line << " -:\t"
	          << message << std::endl;
}

void printF   (Level level, const char* format, ...);
void hexDumpF (Level level, const void* ptr, size_t bytes, const char* format, ...);
void printRF  (fl::Result result, Level threshold, const char* format, ...);
void hexDumpRF(fl::Result result, Level threshold, const void* ptr, size_t bytes, const char* format, ...);

} // fl::logger

} // fl


#define LOGM(LEVEL, MESSAGE)                                                                       \
	do {                                                                                           \
		fl::logger::print(LEVEL, MESSAGE, __FILE__, __LINE__);                                     \
	} while (false)

#define LOGD(MESSAGE) LOGM(fl::logger::LevelSymbol::DEBUG,   MESSAGE)
#define LOGT(MESSAGE) LOGM(fl::logger::LevelSymbol::TRACE,   MESSAGE)
#define LOGI(MESSAGE) LOGM(fl::logger::LevelSymbol::INFO,    MESSAGE)
#define LOGW(MESSAGE) LOGM(fl::logger::LevelSymbol::WARNING, MESSAGE)
#define LOGE(MESSAGE) LOGM(fl::logger::LevelSymbol::ERROR,   MESSAGE)
#define LOGF(MESSAGE) LOGM(fl::logger::LevelSymbol::FATAL,   MESSAGE)

#if defined(_MSC_VER)
// in MS C++ compiler the __FUNCTION__ is a macro string and can be
// concatenated with other strings at the compilation time
	#define FUNCTION_PREFIX(format)          __FUNCTION__ format
	#define FUNCTION_THIS_PREFIX(format)     __FUNCTION__ "[%p]" format, this
#elif defined(__GNUC__)  ||  defined(__clang__)
// in Clang, Xcode and GCC the __FUNCTION__ is a constant, and cannot be concatenated
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
	#define LOG_DETAIL(format, ...)                         fl::logger::printF  (fl::logger::Level::DETAIL, format, ##__VA_ARGS__)
	#define DUMP_DETAIL(ptr, bytes, format, ...)            fl::logger::hexDumpF(fl::logger::Level::DETAIL, ptr, bytes, format, ##__VA_ARGS__)
#else
	#define LOG_DETAIL(format, ...)                         ((void)0)
	#define DUMP_DETAIL(ptr, size, format, ...)             ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_TRACE)
	#define LOG_TRACE(format, ...)                          fl::logger::printF  (fl::logger::Level::TRACE, format, ##__VA_ARGS__)
	#define DUMP_TRACE(ptr, bytes, format, ...)             fl::logger::hexDumpF(fl::logger::Level::TRACE, ptr, bytes, format, ##__VA_ARGS__)
#else
	#define LOG_TRACE(format, ...)                          ((void)0)
	#define DUMP_TRACE(ptr, size, format, ...)              ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_INFO)
	#define LOG_INFO(format, ...)                           fl::logger::printF  (fl::logger::Level::INFO, format, ##__VA_ARGS__)
	#define DUMP_INFO(ptr, bytes, format, ...)              fl::logger::hexDumpF(fl::logger::Level::INFO, ptr, bytes, format, ##__VA_ARGS__)
#else
	#define LOG_INFO(format, ...)                           ((void)0)
	#define DUMP_INFO(ptr, size, format, ...)               ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_WARNING)
	#define LOG_WARNING(format, ...)                        fl::logger::printF  (fl::logger::Level::WARNING, format, ##__VA_ARGS__)
	#define DUMP_WARNING(ptr, bytes, format, ...)           fl::logger::hexDumpF(fl::logger::Level::WARNING, ptr, bytes, format, ##__VA_ARGS__)
#else
	#define LOG_WARNING(format, ...)                        ((void)0)
	#define DUMP_WARNING(ptr, size, format, ...)            ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_ERROR)
	#define LOG_ERROR(format, ...)                          fl::logger::printF  (fl::logger::Level::ERROR, format, ##__VA_ARGS__)
	#define DUMP_ERROR(ptr, bytes, format, ...)             fl::logger::hexDumpF(fl::logger::Level::ERROR, ptr, bytes, format, ##__VA_ARGS__)
#else
	#define LOG_ERROR(format, ...)                          ((void)0)
	#define DUMP_ERROR(ptr, size, format, ...)              ((void)0)
#endif

#if (LOG_THRESHOLD >= LOG_LEVEL_FATAL)
	#define LOG_FATAL(format, ...)                          do { DEBUGBREAK(); fl::logger::printF(fl::logger::Level::FATAL, format, ##__VA_ARGS__);} while (false)
	#define DUMP_FATAL(ptr, bytes, format, ...)             fl::logger::hexDumpF(fl::logger::Level::FATAL, ptr, bytes, format, ##__VA_ARGS__)
#else
	#define LOG_FATAL(format, ...)                          DEBUGBREAK()
	#define DUMP_FATAL(ptr, size, format, ...)              ((void)0)
#endif

#if (LOG_THRESHOLD > LOG_LEVEL_OFF)
	#define LOG_RESULT(result, format, ...)                 fl::logger::printRF  (result, static_cast<fl::logger::Level>(LOG_THRESHOLD), format, ##__VA_ARGS__)
	#define DUMP_RESULT(result, ptr, bytes, format, ...)    fl::logger::hexDumpRF(result, static_cast<fl::logger::Level>(LOG_THRESHOLD), ptr, bytes, format, ##__VA_ARGS__)
#else
	#define LOG_RESULT(result, format, ...)                 ((void)0)
	#define DUMP_RESULT(result, ptr, size, format, ...)     ((void)0)
#endif

#endif // __FL_LOGGER_H__
