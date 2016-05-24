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
 * @file    Result.h
 * @brief   Definition of a common result codes
 */

#pragma once
#ifndef __FL_RESULT_HPP_
#define __FL_RESULT_HPP_

#define RESULT_CODE_BITS            24
#define RESULT_ORIGIN_BITS          4
#define RESULT_SEVERITY_BITS        4

#define RESULT_CODE_OFFSET          0
#define RESULT_ORIGIN_OFFSET        (RESULT_CODE_OFFSET   + RESULT_CODE_BITS)
#define RESULT_SEVERITY_OFFSET      (RESULT_ORIGIN_OFFSET + RESULT_ORIGIN_BITS)

#define RESULT_CODE_MASK            ((1 << RESULT_CODE_BITS)     - 1)
#define RESULT_ORIGIN_MASK          ((1 << RESULT_ORIGIN_BITS)   - 1)
#define RESULT_SEVERITY_MASK        ((1 << RESULT_SEVERITY_BITS) - 1)

#define RESULT_CODE(result)         static_cast<fl::ResultCode>    ((unsigned(result) >> RESULT_CODE_OFFSET)     & RESULT_CODE_MASK)
#define RESULT_ORIGIN(result)       static_cast<fl::ResultOrigin>  ((unsigned(result) >> RESULT_ORIGIN_OFFSET)   & RESULT_ORIGIN_MASK)
#define RESULT_SEVERITY(result)     static_cast<fl::ResultSeverity>((unsigned(result) >> RESULT_SEVERITY_OFFSET) & RESULT_SEVERITY_MASK)

#define MAKE_RESULT_VALUE(severity, origin, code) \
        ((static_cast<unsigned>(severity) << RESULT_SEVERITY_OFFSET) | \
         (static_cast<unsigned>(origin)   << RESULT_ORIGIN_OFFSET)   | \
         (static_cast<unsigned>(code)     << RESULT_CODE_OFFSET))
#define MAKE_RESULT(severity, origin, code) \
        static_cast<fl::Result>(MAKE_RESULT_VALUE(severity, origin, code))

#undef ERROR        // defined in <windows.h>
#undef OVERFLOW     // defined in <math.h>

namespace fl {

enum class ResultSeverity {
	NEUTRAL = 0,
	INFO    = 1,
	WARNING = 2,
	ERROR   = 3,
	FATAL   = 4,
	// always the last
	COUNT
};

enum class ResultOrigin {
	ANY,
	SYSTEM,
	SENSORS,
	INTERNAL,
	// add any used third party library that becomes an origin of returned error codes
	// always the last
	COUNT
}; // fl::ResultOrigin

enum class ResultCode {
	// General
	SUCCESS,                ///! The ideal
	FAILURE,                ///! General failure
	// Specific
	NULL_REFERENCE,         ///! Valid pointer required
	INVALID_ARGUMENT,       ///! Invalid argument
	INVALID_STATE,          ///! Invalid state of an object or module
	INVALID_OBJECT,         ///! Invalid object or module
	NOT_OPENED,             ///! Invalid singleton state: not instantiated
	ALREADY_OPENED,         ///! Invalid singleton state: attempt at subsequent instantiation
	OUT_OF_BOUNDS,          ///! Value out of domain/range
	OUT_OF_MEMORY,          ///! Out of system/heap/device memory
	NOT_FOUND,              ///! Value/object not found
	ALREADY_EXISTS,         ///! Value/object already exists
	TIMEOUT,                ///! Timeout has expired
	RESOURCE_UNAVAILABLE,   ///! Requested resource is not available
	UNSUPPORTED_FEATURE,    ///! Requested feature is not supported
	UNIMPLEMENTED,          ///! Unimplemented code or entire feature
	ALREADY_INITIALIZED,    ///! Module or variable already attached
	UNINITIALIZED,          ///! Uninitialized module or variable
	DIVISION_BY_ZERO,       ///! Division by zero
	UNEXPECTED,             ///! Unexpected condition
	NO_CHANGE,              ///! No change deteted
	ALREADY_ATTACHED,       ///! Already attached to a process
	NOT_ATTACHED,           ///! Not attached to a process
	EMPTY,                  ///! Container is empty
	CORRUPTED_DATA,         ///! Inconsistency detected in the data, possibly caused by transfer malfunction or tampering
	CANCELLED,              ///! Operation has been cancelled
	INCOMPATIBLE,           ///! Incompatible libraries, possible version mismatch
	VERSION_MISMATCH,       ///! Version missmatch
	BROKEN_PIPE,            ///! Broken communication channel
	OVERFLOW,               ///! Buffer or value range overflow
};

enum class Result {
	OK                      = MAKE_RESULT_VALUE(fl::ResultSeverity::NEUTRAL, fl::ResultOrigin::ANY,      ResultCode::SUCCESS),

	I_ALREADY_OPENED        = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::ALREADY_OPENED),
	I_ALREADY_EXISTS        = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::ALREADY_EXISTS),
	I_OUT_OF_BOUNDS         = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::OUT_OF_BOUNDS),
	I_NOT_FOUND             = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::NOT_FOUND),
	I_TIMEOUT               = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::TIMEOUT),
	I_UNSUPPORTED_FEATURE   = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::UNSUPPORTED_FEATURE),
	I_NO_CHANGE             = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::NO_CHANGE),
	I_EMPTY                 = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::EMPTY),
	I_CANCELLED             = MAKE_RESULT_VALUE(fl::ResultSeverity::INFO,    fl::ResultOrigin::INTERNAL, ResultCode::CANCELLED),

	W_NOT_FOUND             = MAKE_RESULT_VALUE(fl::ResultSeverity::WARNING, fl::ResultOrigin::INTERNAL, ResultCode::NOT_FOUND),
	W_TIMEOUT               = MAKE_RESULT_VALUE(fl::ResultSeverity::WARNING, fl::ResultOrigin::INTERNAL, ResultCode::TIMEOUT),
	W_ALREADY_ATTACHED      = MAKE_RESULT_VALUE(fl::ResultSeverity::WARNING, fl::ResultOrigin::INTERNAL, ResultCode::ALREADY_ATTACHED),
	W_ALREADY_INITIALIZED   = MAKE_RESULT_VALUE(fl::ResultSeverity::WARNING, fl::ResultOrigin::INTERNAL, ResultCode::ALREADY_INITIALIZED),
	W_UNINITIALIZED         = MAKE_RESULT_VALUE(fl::ResultSeverity::WARNING, fl::ResultOrigin::INTERNAL, ResultCode::UNINITIALIZED),

	E_FAILURE               = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::FAILURE),
	E_NULL_REFERENCE        = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::NULL_REFERENCE),
	E_INVALID_ARGUMENT      = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::INVALID_ARGUMENT),
	E_INVALID_STATE         = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::INVALID_STATE),
	E_INVALID_OBJECT        = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::INVALID_OBJECT),
	E_NOT_OPENED            = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::NOT_OPENED),
	E_ALREADY_OPENED        = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::ALREADY_OPENED),
	E_OUT_OF_BOUNDS         = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::OUT_OF_BOUNDS),
	E_OUT_OF_MEMORY         = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::OUT_OF_MEMORY),
	E_NOT_FOUND             = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::NOT_FOUND),
	E_TIMEOUT               = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::TIMEOUT),
	E_DIVISION_BY_ZERO      = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::DIVISION_BY_ZERO),
	E_UNIMPLEMENTED         = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::UNIMPLEMENTED),
	E_NOT_ATTACHED          = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::NOT_ATTACHED),
	E_ALREADY_ATTACHED      = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::ALREADY_ATTACHED),
	E_CORRUPTED_DATA        = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::CORRUPTED_DATA),
	E_INCOMPATIBLE          = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::INCOMPATIBLE),
	E_BROKEN_PIPE           = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::BROKEN_PIPE),
	E_OVERFLOW              = MAKE_RESULT_VALUE(fl::ResultSeverity::ERROR,   fl::ResultOrigin::INTERNAL, ResultCode::OVERFLOW),

	F_VERSION_MISMATCH      = MAKE_RESULT_VALUE(fl::ResultSeverity::FATAL,   fl::ResultOrigin::INTERNAL, ResultCode::VERSION_MISMATCH),
	F_UNIMPLEMENTED         = MAKE_RESULT_VALUE(fl::ResultSeverity::FATAL,   fl::ResultOrigin::INTERNAL, ResultCode::UNIMPLEMENTED),
	F_UNEXPECTED            = MAKE_RESULT_VALUE(fl::ResultSeverity::FATAL,   fl::ResultOrigin::INTERNAL, ResultCode::UNEXPECTED)
	// to be continued...

}; // fl::Result

inline Result makeSystemResult(unsigned code)
{
    return MAKE_RESULT(fl::ResultSeverity::ERROR, fl::ResultOrigin::SYSTEM, code);
} // fl::makeSystemResult

#if defined(__linux__)
    #if (defined(__ERRNO_H__) || defined(_SYS_ERRNO_H_) || defined(_GLIBCXX_ERRNO_H) || defined(_ERRNO_H) || defined(_LINUX_ERRNO_H))
        inline Result getSystemResult(void)
        {
            return makeSystemResult(static_cast<unsigned>(errno));
        } // fl::getSystemResult
    #endif
#elif defined(_WIN32) ||  defined(_WIN64)
    #if (defined(_WINDOWS_) || defined(_ERRHANDLING_H_))
        inline Result getSystemResult(void)
        {
            return makeSystemResult(static_cast<unsigned>(GetLastError()));
        } // fl::getSystemResult
    #endif
#endif

} // fl

#endif // __FL_RESULT_HPP_
