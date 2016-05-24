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
 * @file    DebugBreak.h
 * @brief   Software breakpoint. Unlike assertion this one stops the program
 *          execution exectly at the point of call.
 */

#pragma once
#ifndef __FL_DEBUG_BREAK_H__
#define __FL_DEBUG_BREAK_H__

#if !defined(NDEBUG)
	#if defined(_MSC_VER)
		#if defined(_M_IX86) || defined(_M_X64)
			#define DEBUGBREAK()    __debugbreak()
		#elif defined(_M_ARM)
			#define DEBUGBREAK()    do { __asm { bkpt }} while (false)
		#else
			#include <Windows.h>
			#define DEBUGBREAK()    DebugBreak()
		#endif
	#elif defined(__GNUC__)
		#if defined(__x86__) || defined(__x86_64__)
			#define DEBUGBREAK()    asm("int $0x3")
		#elif defined(__arm__)
			#define DEBUGBREAK()    asm("bkpt")
		#else
			#include <signal.h>
			#define DEBUGBREAK()    raise(SIGINT)
		#endif
	#else
		#include <signal.h>
		#define DEBUGBREAK()        raise(SIGINT)
	#endif
#else
	#define DEBUGBREAK()            ((void)0)
#endif

#endif // __FL_DEBUG_BREAK_H__
