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

#pragma once
#ifndef __FL_LOGGER_CONFIG_H__
#define __FL_LOGGER_CONFIG_H__

//! Choose single or multiple output as disjunction of the following flags:
//! LOG_OUTPUT_STDOUT = Standard output (console)
//! LOG_OUTPUT_SYSDBG = System debug stream (stderr on Linux, DbgPrint on Windows, logcat on Android, dlog on Tizen, NSLog on iOS,...)
//! LOG_OUTPUT_FILE   = Cycle of files with upper limit in size
#define LOG_OUTPUT                  LOG_OUTPUT_FILE

//! Choose operation scope using one of the following values:
//! LOG_RANGE_MACHINE = Single log for all processes within node (slowest, logs collected from separate processes into single output)
//! LOG_RANGE_PROCESS = Single log for all threads within process (moderate exclusion overhead, multi-threaded output)
//! LOG_RANGE_THREAD  = Separate logs for every thread (fastest, single-thnreaded outputs; potentially produces lots of files and may result in conflicts when writing to stdout & stddbg)
#define LOG_RANGE                   LOG_RANGE_PROCESS

//! Choose time resolution as one of the following:
//! LOG_TIME_RESOLUTION_MS = milliseconds
//! LOG_TIME_RESOLUTION_US = microseconds
//! LOG_TIME_RESOLUTION_NS = nanoseconds
#define LOG_TIME_RESOLUTION         LOG_TIME_RESOLUTION_MS

//! Define the maximal number of remembered contexts (indentation and files, if used)
#define LOG_THREAD_CONTEXTS         16

//! Define the limit on the number of indents (tabs, arbitrary)
#define LOG_MAX_INDENT              8

//! Define the limit on the length of formatted message (characters, arbitrary)
#define MAX_MESSAGE_LENGTH          256

//! Define the limit on the length of hexadecimal memory dump (characters, less than MAX_MESSAGE_LENGTH)
#define MAX_DUMP_LENGTH             192

//! Select whether threads shall be separated with empty line (1) or not (0)
#define LOG_SEPARATE_THREADS        1

#if (LOG_OUTPUT&LOG_OUTPUT_FILE)
    //! Define the number of files in a cyclic file buffer
    #define LOG_FILE_CYCLE          4

    //! Define the maximal size in bytes of a single log file within a cycle
    #define LOG_FILE_SIZE_LIMIT     2*1024*1024

    //! Define location where the log file will be created (note, that the
    //! logger does not create this directory, so make sure it exists if
    //! you set the output file). The string should end with OS-specific
    //! directory separator.
    #if defined(__linux__)
        #define LOG_FILE_DIR        "/var/log/"
    #else // Windows
        #define LOG_FILE_DIR        "H:\\project\\ctxfw\\log\\"
    #endif

    #if (LOG_RANGE == LOG_RANGE_MACHINE)
        //! Base file name used in multi-process logging
        #define LOG_FILE_BASENAME   "ctxfw."
    #endif
#endif

#if (LOG_OUTPUT&LOG_OUTPUT_STDOUT)
    //! Choose colorization scheme using one of the following values:
    //! LOG_COLOR_SCHEME_OFF  = Colorization turned off
    //! LOG_COLOR_SCHEME_DARK  = Colorization assuming black background
    //! LOG_COLOR_SCHEME_LIGHT = Colorization assuming white background
    #if defined(__linux__)
        // xterm supports ANSI colors
        #define LOG_COLOR_SCHEME    LOG_COLOR_SCHEME_DARK
    #else
        // windows console does not (but telnet does)
        #define LOG_COLOR_SCHEME    LOG_COLOR_SCHEME_OFF
    #endif
#endif

#endif // __FL_LOGGER_CONFIG_H__
