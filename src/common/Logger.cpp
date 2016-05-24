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

#define SHARED_LOG_DEBUG            0
#define SHARED_LOG_DEBUG_OVERFLOWS  (1&SHARED_LOG_DEBUG)
#define SHARED_LOG_DEBUG_THREADS    (1&SHARED_LOG_DEBUG)

//! This is a no-go: (i) missing conversion from std::thread::id to a printable number, (ii) missing null id
#define C11_THREADS         0 // 0=disablle, 1=enable

//! For discussion and comparison of performance between std::mutex and CRITICAL_SECTION see e.g.
//! https://social.msdn.microsoft.com/Forums/vstudio/en-US/3a9387ed-f15f-4b80-944e-180f41980a2c/stdmutex-performance-compared-to-win32-mutex-or-critical-section-apis?forum=vcgeneral
//! http://stackoverflow.com/questions/9997473/stdmutex-performance-compared-to-win32-critical-section
#define C11_MUTEX           1 // 1=enable

#if (__linux__ || ANDROID_NDK)
	#define C11_TIMESTAMP   1 // 0=disablle, 1=enable
#else // Windows
	//! Beware of crappy implementation of std::chrono::high_precision_clock on Windows, which gives granularity ~20ms (sic!), and significant execution drag (sic^2!)
	#define C11_TIMESTAMP   0 // 0=disablle, 1=enable
#endif

#if (defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS))
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#if (C11_THREADS)
	#include <thread>
#endif
#include <mutex>
#include <system_error>
#if (C11_TIMESTAMP)
	#include <ctime>
	#include <chrono>
	using Clock = std::chrono::high_resolution_clock;
#endif
#if (__linux__ || ANDROID_NDK)
	#include <sys/stat.h>
	#include <sys/file.h>
	#include <pthread.h>
	#include <sys/syscall.h>
	#if (!C11_TIMESTAMP)
		#include <time.h>
		#include <sys/time.h>
	#endif
	#include <unistd.h>
	#include <fcntl.h>
	#include <cstring>
	#include <linux/limits.h>
	#if (!C11_THREADS)
		typedef pid_t               ThreadId;
	#endif
	typedef int                     FileHandle;
	typedef uint8_t                 byte;
	#define INVALID_HANDLE_VALUE    0
	#define MAX_PATH                PATH_MAX
	#if (ANDROID_NDK)
		#include <linux/stat.h>
		#include <android/log.h>
	#elif (TIZEN_NDK)
		#include <dlog.h>
	#else
		#include <sys/types.h>
		#include <linux/stat.h>
	#endif
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE         /* See feature_test_macros(7) */
	#endif
	#include <errno.h>
	#if (LOG_OUTPUT&LOG_OUTPUT_FILE)
		extern char *program_invocation_short_name;
	#endif
#elif defined(_WIN32) ||  defined(_WIN64)
	#define vsnprintf       _vsnprintf
	#define snprintf        _snprintf
	#include <windows.h>
	#if (!C11_THREADS)
		typedef DWORD   ThreadId;
	#endif
	typedef HANDLE      FileHandle;
#else
	#error Unsupported OS
#endif

#if (C11_THREADS)
	typedef std::thread::native_handle_type ThreadId;
#endif

#include "Result.h"
#include "Logger.h"

//! Indentation is done with ASCII TAB code
#define LOG_INDENT_CHAR             '\t'

#define TEXTSIZE(t)                 (sizeof(t) - 1)

//! Standard output (console)
#define LOG_OUTPUT_STDOUT           0x1
//! System debug stream (stderr on Linux, DbgPrint on Windows, logcat on Android, NSLog on iOS, ...)
#define LOG_OUTPUT_SYSDBG           0x2
//! Cycle of files with upper limit in size
#define LOG_OUTPUT_FILE             0x4

//! Single file for all logs (slowest)
#define LOG_RANGE_MACHINE           0
//! Single file for all threads within process
#define LOG_RANGE_PROCESS           1
//! Separate file for every thread
#define LOG_RANGE_THREAD            2

//! Milliseconds' time resolution
#define LOG_TIME_RESOLUTION_MS      1000
//! Microseconds' time resolution
#define LOG_TIME_RESOLUTION_US      1000000
//! Namoseconds' time resolution
#define LOG_TIME_RESOLUTION_NS      1000000000

//! Colorization turned off
#define LOG_COLOR_SCHEME_OFF        0
//! Colorization assuming black background
#define LOG_COLOR_SCHEME_DARK       1
//! Colorization assuming white background
#define LOG_COLOR_SCHEME_LIGHT      2

#include "LoggerConfig.h"

#if ((LOG_OUTPUT&LOG_OUTPUT_STDOUT) == 0)
	#undef  LOG_COLOR_SCHEME
	#define LOG_COLOR_SCHEME        LOG_COLOR_SCHEME_OFF
#endif
// sanity check
#if (LOG_THREAD_CONTEXTS < 1)
	#error LOG_THREAD_CONTEXTS < 1
#endif

#if (SHARED_LOG_DEBUG_OVERFLOWS)
	#define         SENTINEL_LENGTH 4
#else
	#define         SENTINEL_LENGTH 0
#endif
#define            LOG_LEVEL_LENGTH 1
#if (LOG_TIME_RESOLUTION == LOG_TIME_RESOLUTION_MS)
	#define        TIMESTAMP_LENGTH 12  // strlen("HH:MM:SS.mmm")
#elif (LOG_TIME_RESOLUTION == LOG_TIME_RESOLUTION_US)
	#define        TIMESTAMP_LENGTH 16  // strlen("HH:MM:SS.mmm.uuu")
#elif (LOG_TIME_RESOLUTION == LOG_TIME_RESOLUTION_NS)
	#define        TIMESTAMP_LENGTH 20  // strlen("HH:MM:SS.mmm.uuu.nnn")
#else
	#error Unsupported LOG_TIME_RESOLUTION
#endif
#if defined(__linux__)
	#define              EOL_LENGTH 1   // strlen("\n")
#else
	#define              EOL_LENGTH 2   // strlen("\r\n")
#endif
#define            THREAD_ID_LENGTH (sizeof(ThreadId)*2)
#define             ELLIPSIS_LENGTH 3   // strlen("...")
#define                  NUL_LENGTH 1   // strlen("\0")
#define                  SPC_LENGTH 1   // strlen(" ")
#define                  TAB_LENGTH 1   // strlen("\t")
#define               INDENT_LENGTH TAB_LENGTH
#define       LOG_MAX_INDENT_LENGTH (LOG_THREAD_CONTEXTS*INDENT_LENGTH)
#if (LOG_SEPARATE_THREADS)
	#define THREAD_SEPARATOR_LENGTH EOL_LENGTH
#else
	#define THREAD_SEPARATOR_LENGTH 0
#endif
#if (LOG_COLOR_SCHEME == LOG_COLOR_SCHEME_OFF)
	#define        COLOR_SET_LENGTH 0
	#define      COLOR_RESET_LENGTH 0
#else
	#define        COLOR_SET_LENGTH TEXTSIZE(c_severityColorCmd[1])  // longest of the color-setting sequences
	#define      COLOR_RESET_LENGTH TEXTSIZE(c_resetColorCmd)
#endif
#define                 LINE_LENGTH (THREAD_SEPARATOR_LENGTH + LOG_LEVEL_LENGTH + SPC_LENGTH + TIMESTAMP_LENGTH + SPC_LENGTH + THREAD_ID_LENGTH + SPC_LENGTH + MAX_MESSAGE_LENGTH)
#define               BUFFER_LENGTH (COLOR_SET_LENGTH + LINE_LENGTH + EOL_LENGTH + COLOR_RESET_LENGTH + NUL_LENGTH + SENTINEL_LENGTH)

#if (MAX_MESSAGE_LENGTH <= ELLIPSIS_LENGTH)
	// 4 = length of the ellipsis "..."
	#error MAX_MESSAGE_LENGTH too small
#endif

#if (LOG_RANGE == LOG_RANGE_MACHINE)
	#define MUTEX_RANGE         LOG_RANGE_MACHINE
#elif (LOG_RANGE == LOG_RANGE_PROCESS)
	#define MUTEX_RANGE         LOG_RANGE_PROCESS
#elif (LOG_RANGE == LOG_RANGE_THREAD)
	#if (LOG_OUTPUT&(LOG_OUTPUT_STDOUT | LOG_OUTPUT_SYSDBG))
		#define MUTEX_RANGE     LOG_RANGE_PROCESS
	#else
		#define MUTEX_RANGE     LOG_RANGE_THREAD
	#endif
#else
	#error Unsupported LOG_RANGE
#endif

namespace fl {
namespace logger {

/**************************************************************//**
 *  @class fl::logger::Singleton
 *
 *  Main class of the logger. It is instantiated and killed outside
 *  the main function in order to facilitate early-on logging.
 *  There's no need to initialize manually.
 */
class Singleton {
public:
		//! Constructor
		Singleton(void);
		//! Destructor
	~Singleton();

	//! Format given string (printf compatible), and write the result into the output(s) specified by
	//! LOG_OUTPUT
	void printF(Level level, const char* format, va_list vaArgs);
	//! Create hexadecimal string of the form {01 23 45 67 89 AB CD EF}, compose it with the given format
	//! and arguments, and write the result into the output(s) specified by LOG_OUTPUT
	void hexDumpF(Level level, const void* ptr, size_t bytes, const char* format, va_list vaArgs);

private: // Data structures
#if (LOG_OUTPUT&LOG_OUTPUT_FILE)
	/**************************************************************//**
	 *  @class fl::logger::Singleton::FileCycle
	 *
	 *  Encapsulates basic operations performed when logging into a
	 *  file. The class is multiply-instantiated only if LOG_RANGE
	 *  == LOG_RANGE_THREAD.
	 */
	class FileCycle {
	public:
		//! Constructor
		FileCycle(void);
		//! Destructor
		~FileCycle();
		//! Open cycle of log files with the given @baseName
		inline fl::Result Open(void)
		{
			return _Open(0);
		}
		//! Close the cycle
		inline void Close(void);
		//! Write given number of @a bytes of given @a output into the file cycle.
		fl::Result Write(const char* output, size_t bytes);
		//! Flush the output buffers
		inline void Flush(void);

	private:
		//! Return name of the process used for construction of the log file
		static const char* _BaseName(void);
		//! Open cycle of log files with the given @baseName
		fl::Result _Open(unsigned cycleIndex);
		//! Close current file in the cycle
		void _Close(void);

		static char s_baseName[MAX_PATH];
		FileHandle  m_handle;     //!< current file handle
		size_t      m_size;       //!< current file size in bytes
		unsigned    m_cycleIndex; //!< current file cycle index
	}; // fl::logger::Singleton::FileCycle
#endif

	/**************************************************************//**
	 *  @class fl::logger::Singleton::Mutex
	 *
	 *  Implementation of the mutex depends on the operating mode:
	 *      - inter-process for global range
	 *      - inter-thread for process range
	 *      - void for single thread range
	 */
	class Mutex
	#if (MUTEX_RANGE == LOG_RANGE_PROCESS)
	: protected std::mutex
	#endif
	{
	public:
		//! Constructor
		Mutex(void);
		//! Destructor
		~Mutex();

		//! Enter exclusion zone
	#if (MUTEX_RANGE == LOG_RANGE_THREAD)
		inline fl::Result Lock(void) {
			return Result::OK;
		}
	#else
		fl::Result Lock(void);
	#endif

		//! Leave exclusion zone
	#if (MUTEX_RANGE == LOG_RANGE_THREAD)
		inline void Unlock(void) { }
	#elif (MUTEX_RANGE == LOG_RANGE_PROCESS)
		inline void Unlock(void) {
			std::mutex::unlock();
		}
	#else
		void Unlock(void);
	#endif
	private:
	#if (MUTEX_RANGE == LOG_RANGE_MACHINE)
		static const char* GLOBAL_MUTEX_NAME;
		FileHandle         m_handle;
	#endif
	}; // fl::logger::Singleton::Mutex

	/**************************************************************//**
	 *  @class fl::logger::Singleton::ThreadContext
	 *
	 *  Implementation of the indent and separation of file outputs.
	 */
	struct ThreadContext {
		ThreadId       threadId;
		unsigned       indent;
		ThreadContext* pNext;
	#if ((LOG_OUTPUT&LOG_OUTPUT_FILE) && (LOG_RANGE == LOG_RANGE_THREAD))
		FileCycle      fileCycle;
	#endif
	}; // fl::logger::Singleton::ThreadContext

	/**************************************************************//**
	 *  @class fl::logger::Singleton::TimeStamp
	 */
	class TimeStamp {
	public:
		//! Constructor
		TimeStamp(void);
		//! Pick up and remember the present moment of time
		void Catch(void);
		//! Output the remembered time as human-readable, resolution-dependent form
		unsigned Format(char* output);

	private: // Types
		//! Time point's units are defined by LOG_TIME_RESOLUTION: {ms, us, ns}
		typedef uint64_t TimePoint;

	private: // Data
				TimePoint m_timeOffset;
	#if (C11_TIMESTAMP)
		Clock::time_point m_now;
		Clock::time_point m_ref;
	#else
				TimePoint m_now;
		#if defined(_WIN32) ||  defined(_WIN64)
					double m_timeUnitsPerCount;
		#endif
	#endif
	}; // fl::logger::Singleton::TimeStamp

private: // Methods
	//!
	void _SwitchThreadContext(ThreadId threadId);

private: // Data
	ThreadContext  m_threadContext[LOG_THREAD_CONTEXTS];
	ThreadContext* m_pCurrentThreadContext;
		TimeStamp  m_timeStamp;
			Mutex  m_mutex;
#if ((LOG_OUTPUT&LOG_OUTPUT_FILE) && (LOG_RANGE != LOG_RANGE_THREAD))
		FileCycle  m_fileCycle;
#endif
}; // fl::logger::Singleton

static inline ThreadId _GetCallingThreadId(void);

//! Lookup table from half-bytes onto ASCII representation of hexadecimal code
static const char c_hexCode[0x10] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

//! Lookup table from level values into their ASCII representation
static const char c_levelSymbol[size_t(Level::COUNT)] = {
	'?', // Level::OFF
	'#', // Level::FATAL
	'E', // Level::ERROR
	'!', // Level::WARNING
	'i', // Level::INFO
	'.', // Level::TRACE
	' '  // Level::DETAIL
};

#if (LOG_OUTPUT&LOG_OUTPUT_SYSDBG)
#if (ANDROID_NDK)
	#define LOG_TAG "CTXFW"
	//! Lookup table from level values onto logcat priorities
	static const int c_androidPriority[size_t(Level::COUNT)] = {
		ANDROID_LOG_SILENT, // Level::OFF
		ANDROID_LOG_FATAL,  // Level::FATAL
		ANDROID_LOG_ERROR,  // Level::ERROR
		ANDROID_LOG_WARN,   // Level::WARNING
		ANDROID_LOG_INFO,   // Level::INFO
		ANDROID_LOG_DEBUG,  // Level::TRACE
		ANDROID_LOG_VERBOSE // Level::DETAIL
	};
#elif (TIZEN_NDK)
	#define LOG_TAG "CTXFW"
	//! Lookup table from level values onto dlog priorities
	static const int c_tizenPriority[size_t(Level::COUNT)] = {
		DLOG_SILENT, // Level::OFF
		DLOG_FATAL,  // Level::FATAL
		DLOG_ERROR,  // Level::ERROR
		DLOG_WARN,   // Level::WARNING
		DLOG_INFO,   // Level::INFO
		DLOG_DEBUG,  // Level::TRACE
		DLOG_VERBOSE // Level::DETAIL
	};
#endif
#endif

//! Lookup table from severity values into levels
static const Level c_severityLevel[size_t(fl::ResultSeverity::COUNT)] = {
	Level::TRACE,   // fl::ResultSeverity::NEUTRAL
	Level::INFO,    // fl::ResultSeverity::INFO
	Level::WARNING, // fl::ResultSeverity::WARNING
	Level::ERROR,   // fl::ResultSeverity::ERROR
	Level::FATAL    // fl::ResultSeverity::FATAL
};

#if (LOG_COLOR_SCHEME == LOG_COLOR_SCHEME_DARK)
static const char* c_severityColorCmd[size_t(Level::COUNT)] = {
	"",                 // -            Level::OFF
	"\33[1;35m",        // magenta      Level::FATAL
	"\33[1;31m",        // red          Level::ERROR
	"\33[1;33m",        // yellow       Level::WARNING
	"\33[1;37m",        // white        Level::INFO
	"\33[1;32m",        // light green  Level::TRACE
	"\33[1;36m",        // light cyan   Level::DETAIL
};
static const char  c_resetColorCmd[] = "\33[2;37m"; // light gray
#elif (LOG_COLOR_SCHEME == LOG_COLOR_SCHEME_LIGHT)
static const char* c_severityColorCmd[size_t(Level::COUNT)] ={
	"",                 // -            Level::OFF
	"\33[2;30m",        // black        Level::FATAL
	"\33[1;31m",        // red          Level::ERROR
	"\33[2;35m",        // purple       Level::WARNING
	"\33[1;34m",        // blue         Level::INFO
	"\33[2;36m",        // teal         Level::TRACE
	"\33[1;30m",        // dark gray    Level::DETAIL
};
static const char  c_resetColorCmd[] = "\33[1;30m"; // dark gray
#endif

static Singleton singleton;

Singleton::Singleton(void)
: 	m_pCurrentThreadContext(&m_threadContext[0])
,   m_timeStamp()
, 	m_mutex()
#if ((LOG_OUTPUT&LOG_OUTPUT_FILE) && (LOG_RANGE != LOG_RANGE_THREAD))
, 	m_fileCycle()
#endif
{
#if ((LOG_OUTPUT&LOG_OUTPUT_FILE) && (LOG_RANGE != LOG_RANGE_THREAD))
	m_fileCycle.Open();
#endif
#if (SHARED_LOG_DEBUG)
	#if (__linux__ || ANDROID_NDK)
		printf("fl::logger::Singleton::Singleton[%p](), pid=%08X\r\n", this, unsigned(syscall(SYS_getpid)));
	#else // Windows
		printf("fl::logger::Singleton::Singleton[%p](), pid=%08X\r\n", this, GetCurrentProcessId());
	#endif
#endif
	for (unsigned i = LOG_THREAD_CONTEXTS;  i--;) {
		m_threadContext[i].threadId = 0;
		m_threadContext[i].indent   = 0;
		m_threadContext[i].pNext    = (i < LOG_THREAD_CONTEXTS - 1 ? &m_threadContext[i + 1] : nullptr);
	}
} // fl::logger::Singleton::Singleton

///////////////////////////////////////////////////////////////////////
//  [internal] Module exit
fl::logger::Singleton::~Singleton()
{
#if (SHARED_LOG_DEBUG)
	#if (__linux__ || ANDROID_NDK)
		printf("fl::logger::Singleton::~Singleton[%p](), pid=%08X\r\n", this, unsigned(syscall(SYS_getpid)));
	#else // Windows
		printf("fl::logger::Singleton::Singleton[%p](), pid=%08X\r\n", this, GetCurrentProcessId());
	#endif
#endif
#if (LOG_OUTPUT&LOG_OUTPUT_FILE)
	#if (LOG_RANGE == LOG_RANGE_THREAD)
	for (unsigned i = LOG_THREAD_CONTEXTS; i--;) {
		m_threadContext[i].fileCycle.Close();
	}
	#else
	m_fileCycle.Close();
	#endif
#endif
} // fl::logger::Singleton::~Singleton

///////////////////////////////////////////////////////////////////////
// [private]
#if (SHARED_LOG_DEBUG_THREADS)
ThreadId _GetCallingThreadId(void)
{
#define SIMULATED_THREADS   (1 << 2)
#define RETURN_LAST         0x030
#define RETURN_NEW          0x1C0
	static ThreadId s_threadId_[SIMULATED_THREADS] = {1, 2, 3, 4};
	static ThreadId s_lastThread = 1;
	unsigned i, r;

	r = rand();
	if (r&RETURN_LAST)
		return s_lastThread;
	else {
		i = r&(SIMULATED_THREADS - 1);
		if ((r&RETURN_NEW) == 0)
			s_threadId_[i] = ThreadId(rand());
		return s_lastThread = s_threadId_[i];
	}
} // fl::logger::_GetCallingThreadId
#else
inline ThreadId _GetCallingThreadId(void)
{
#if (C11_THREADS)
	return std::this_thread::native_handle();
#else
	#if (__linux__ || ANDROID_NDK)
		return (ThreadId)syscall(SYS_gettid);
	#else // Windows
		return GetCurrentThreadId();
	#endif
#endif
} // fl::logger::_GetCallingThreadId
#endif

///////////////////////////////////////////////////////////////////////
//  [private]
void Singleton::_SwitchThreadContext(ThreadId threadId)
{
	ThreadContext* pTC;
	ThreadContext* pPriorTC;
	ThreadContext* pNextTC;

	pPriorTC = nullptr;
	pTC      = m_pCurrentThreadContext;
#if (SHARED_LOG_DEBUG_THREADS)
	unsigned i = 0;
#endif
	do {
		pNextTC = pTC->pNext;
		if (pTC->threadId == threadId) {
			// found it on the list
			break;
		}
		else if (pTC->threadId == 0) {
			// pick up an unused context. if we're here the thread is not on the list
			pTC->threadId = threadId;
		#if ((LOG_OUTPUT&LOG_OUTPUT_FILE) && (LOG_RANGE == LOG_RANGE_THREAD))
			pTC->fileCycle.Open();
		#endif
			break;
		}
		if (pNextTC == nullptr) {
		#if (SHARED_LOG_DEBUG_THREADS)
			if (i != LOG_THREAD_CONTEXTS - 1)
				printf("FATAL: fl::logger::Singleton::_SwitchThreadContext(0x%X): thread loss detected\r\n", threadId);
		#endif
		#if ((LOG_OUTPUT&LOG_OUTPUT_FILE) && (LOG_RANGE == LOG_RANGE_THREAD))
			pTC->fileCycle.Close();
		#endif
			// we're recycling an old (least used at least) context
			pTC->threadId = threadId;
			pTC->indent   = 0;
		#if ((LOG_OUTPUT&LOG_OUTPUT_FILE) && (LOG_RANGE == LOG_RANGE_THREAD))
			pTC->fileCycle.Open();
		#endif
			break;
		}
		pPriorTC = pTC;
		pTC = pNextTC;
	#if (SHARED_LOG_DEBUG_THREADS)
		if (++i >= LOG_THREAD_CONTEXTS) {
			printf("FATAL: fl::logger::Singleton::_SwitchThreadContext(0x%X): unterminated list\r\n", threadId);
			break;
		}
	#endif
	}
	while (true);
	// move to the head of the list
	if (pTC != m_pCurrentThreadContext) {
		if (pPriorTC)
			pPriorTC->pNext = pNextTC;
		pTC->pNext = m_pCurrentThreadContext;
		m_pCurrentThreadContext = pTC;
	}
#if (SHARED_LOG_DEBUG_THREADS)
	for (i = LOG_THREAD_CONTEXTS, pTC = m_pCurrentThreadContext;  i > 0;  i--, pTC = pNextTC) {
		pNextTC = pTC->pNext;
		if (pNextTC == nullptr) {
			if (i != 1) {
				DEBUGBREAK();
				printf("FATAL: fl::logger::Singleton::_SwitchThreadContext(): premature list termination\n");
			}
			return;
		}
	}
	DEBUGBREAK();
	printf("FATAL: fl::logger::Singleton::_SwitchThreadContext(): circular list\n");
#endif
} // fl::logger::Singleton::_SwitchThreadContext

/**********************************************************************//**
 *  [private] This is the main logging function: it formats the message and
 *  sends it to the configured output streams. Each line is appended with
 *  the EOL character (LF), unles it already has one. Lines exceeding
 *  configured length (MAX_MESSAGE_LENGTH) are truncated with ellipsis. The
 *  format is as follows:
 *      L HH:MM:SS.mmm.uuu TTTTTTTT <your message>\n
 *  where
 *      L                = level symbol
 *      HH:MM:SS.mmm.uuu = local time
 *      TTTTTTTT         = thread ID
 *
 *  @param [in] level
 *      Importance/severity level
 *  @param [in] format
 *      ANSI-C string format (printf style)
 *  @param [in] vaArgs
 *      List of variadic arguments to the format
 */
void fl::logger::Singleton::printF(Level level, const char* format, va_list vaArgs)
{
	unsigned sysdbgLength(0);
	// Seize the moment before waiting on mutex, to get more accurate time of the event
	m_timeStamp.Catch();

	if (m_mutex.Lock() == Result::OK) {
			char  stdoutLine[BUFFER_LENGTH];
			char* sysdbgLine(stdoutLine);
			char* fileLine  (stdoutLine);
		ThreadId  threadId(_GetCallingThreadId());
		unsigned  i;

	#if (SENTINEL_LENGTH > 0)
		for (i = SENTINEL_LENGTH;  i--;)
			stdoutLine[BUFFER_LENGTH - 1 - i] = char(-1);
	#endif
	#if ((LOG_OUTPUT&LOG_OUTPUT_STDOUT) && (LOG_COLOR_SCHEME != LOG_COLOR_SCHEME_OFF))
	// colors are not written into debug stream and files
	{
		// foreground color
		const char* colorCmd;

		colorCmd = c_severityColorCmd[unsigned(level)];
		for (i = 0;  (*sysdbgLine = colorCmd[i]) != '\0';  i++, sysdbgLine++) { }
	}
	#endif
		fileLine = sysdbgLine;
		if (m_pCurrentThreadContext->threadId != threadId) {
		#if (LOG_SEPARATE_THREADS)
			// don't start with newline
			if (m_pCurrentThreadContext->threadId != 0) {
			#if !defined(__linux__)
				sysdbgLine[sysdbgLength++] = '\r';
			#endif
				sysdbgLine[sysdbgLength++] = '\n';
			}
			#if (LOG_RANGE == LOG_RANGE_THREAD)
				// don't write EOL separator if we already separate files
				fileLine += EOL_LENGTH;
			#endif
		#endif
			// retrieve past or create new thread context (indent)
			_SwitchThreadContext(threadId);
		}

		// level
		sysdbgLine[sysdbgLength++] = fl::logger::c_levelSymbol[unsigned(level)];
		sysdbgLine[sysdbgLength++] = ' ';

		// time-stamp
		sysdbgLength += m_timeStamp.Format(sysdbgLine + sysdbgLength);
		sysdbgLine[sysdbgLength++] = ' ';

		// thread id
		for (i = THREAD_ID_LENGTH; i;) {
			i--;
			sysdbgLine[sysdbgLength + i] = c_hexCode[threadId&0xF];
			threadId >>= 4;
		}
		sysdbgLength += THREAD_ID_LENGTH;
		sysdbgLine[sysdbgLength++] = ' ';

		// unindent?
		if (format[0] == LOG_UNINDENT_PREFIX[0])
			if (m_pCurrentThreadContext->indent > 0)
				m_pCurrentThreadContext->indent--;

		for (i = m_pCurrentThreadContext->indent;  i > 0;  --i)
			sysdbgLine[sysdbgLength++] = LOG_INDENT_CHAR;
		// format message
		int result;
		int freeSpace;

		// in case of error, terminate the string
		sysdbgLine[sysdbgLength] = '?';
		sysdbgLine[sysdbgLength + 1] = '\0';
		freeSpace = LINE_LENGTH - sysdbgLength;
		// the vsnprintf() behaves differently on Windows and Linux: on Windows
		// negative result indicates overflow, on Linux - some other errors.
		result = vsnprintf(sysdbgLine + sysdbgLength, freeSpace, format, vaArgs);
		if (result >= 0 && result < freeSpace) {
			// normal case
			sysdbgLength += unsigned(result);
			// remove trailing newline for consistency
			if (sysdbgLine[sysdbgLength - 1] == '\n')
				sysdbgLength--;
		#if !defined(__linux__)
			if (sysdbgLine[sysdbgLength - 1] == '\r')
				sysdbgLength--;
		#endif
			sysdbgLine[sysdbgLength] = '\0';
		}
		else {
			// add ellipsis on overflow
			sysdbgLine[LINE_LENGTH - 3] = '.';
			sysdbgLine[LINE_LENGTH - 2] = '.';
			sysdbgLine[LINE_LENGTH - 1] = '.';
			sysdbgLine[LINE_LENGTH - 0] = '\0';
			sysdbgLength = LINE_LENGTH;
		}
		// indent?
		if (format[0] == LOG_INDENT_PREFIX[0]) {
			if (m_pCurrentThreadContext->indent < LOG_MAX_INDENT)
				m_pCurrentThreadContext->indent++;
		}

	#if (LOG_OUTPUT&LOG_OUTPUT_STDOUT)
	{
		unsigned stdoutLength = sysdbgLength + unsigned(sysdbgLine - stdoutLine);
		// append newline (before color reset to extend the background)
	#if !defined(__linux__)
		stdoutLine[stdoutLength++] = '\r';
	#endif
		stdoutLine[stdoutLength++] = '\n';
		#if (LOG_COLOR_SCHEME != LOG_COLOR_SCHEME_OFF)
			// append color reset
			for (unsigned i = 0;  (stdoutLine[stdoutLength] = c_resetColorCmd[i]) != 0;  i++, stdoutLength++) { }
		#else
			stdoutLine[stdoutLength] = 0;
		#endif
		fwrite(stdoutLine, stdoutLength, sizeof(char), stdout);
		// restore termination
		sysdbgLine[sysdbgLength] = 0;
	}
	#endif

	#if (LOG_OUTPUT&LOG_OUTPUT_SYSDBG)
		#if (ANDROID_NDK)
			// newlines are automatically added by logcat
			__android_log_write(c_androidPriority[level], LOG_TAG, sysdbgLine);
		#elif (TIZEN_NDK)
			// newlines are automatically added by logcat
			dlog_print(c_tizenPriority[level], LOG_TAG, sysdbgLine);
		#else
			// append newline
		#if defined(__linux__)
			sysdbgLine[sysdbgLength + 0] = '\n';
			sysdbgLine[sysdbgLength + 1] = '\0';
		#else
			sysdbgLine[sysdbgLength + 0] = '\r';
			sysdbgLine[sysdbgLength + 1] = '\n';
			sysdbgLine[sysdbgLength + 2] = '\0';
		#endif
			#if (__linux__ || ANDROID_NDK)
				fwrite(sysdbgLine, sysdbgLength + EOL_LENGTH, sizeof(char), stderr);
			#else // Windows
				OutputDebugStringA(sysdbgLine);
			#endif
			// restore termination
			sysdbgLine[sysdbgLength] = 0;
		#endif
	#endif
	#if (LOG_OUTPUT&LOG_OUTPUT_FILE)
	{
		unsigned fileLength = sysdbgLength - unsigned(fileLine - sysdbgLine);
		#if defined(__linux__)
			fileLine[fileLength + 0] = '\n';
			fileLine[fileLength + 1] = '\0';
		#else
			fileLine[fileLength + 0] = '\r';
			fileLine[fileLength + 1] = '\n';
			fileLine[fileLength + 2] = '\0';
		#endif
		#if (LOG_RANGE == LOG_RANGE_THREAD)
			m_pCurrentThreadContext->fileCycle.Write(fileLine, fileLength + EOL_LENGTH);
		#else
			m_fileCycle.Write(fileLine, fileLength + EOL_LENGTH);
		#endif
		// restore termination
		sysdbgLine[sysdbgLength] = '\0';
	}
	#endif
	#if (SENTINEL_LENGTH > 0)
		for (i = SENTINEL_LENGTH;  i--;) {
			if (stdoutLine[BUFFER_LENGTH - 1 - i] != char(-1)) {
				DEBUGBREAK();
				printf("FATAL: fl::logger::VPrintF() stdoutLine overrun\n");
			}
		}
	#endif
		m_mutex.Unlock();
	}
} // fl::logger::Singleton::printF

/**************************************************************************//**
 *  [public] @todo This method is not properly implemented yet - don't use.
 *
 *  @param [in] level
 *      Logging level
 *  @param [in] ptr
 *      Beginning address of the memory
 *  @param [in] bytes
 *      Number of bytes to be dumped.
 *  @param [in] format
 *      Mesage format
 *  @param [in] vaArgs
 */
void fl::logger::Singleton::hexDumpF(Level level, const void* ptr, size_t bytes, const char* format, va_list vaArgs)
{
			char  stdoutLine[MAX_DUMP_LENGTH];
		size_t  written = 0;
			byte  b;
	const byte* pb = reinterpret_cast<const byte*>(ptr);

	if (ptr) {
		stdoutLine[written++] = '{';
		if (bytes) {
			do {
				b = *pb++;
				stdoutLine[written++] = c_hexCode[b >> 4];
				stdoutLine[written++] = c_hexCode[b & 0xF];
				if (--bytes == 0) {
					break;
				}
				stdoutLine[written++] = ' ';
				if (sizeof(stdoutLine) < written + 3 + 2) {
					// no room for another byte (3) and closing bracket (1): terminate with ellipsis
					stdoutLine[sizeof(stdoutLine) - 6] = '.';
					stdoutLine[sizeof(stdoutLine) - 5] = '.';
					stdoutLine[sizeof(stdoutLine) - 4] = '.';
					// closing bracket and terminating zero
					written = sizeof(stdoutLine) - 3;
					break;
				}
			}
			while (true);
		}
		stdoutLine[written++] = '}';
		stdoutLine[written]   = '\0';
	}
	else
		strncpy(stdoutLine, "(null)", sizeof(stdoutLine));

	// @todo print the hex buffer instead
	printF(level, format, vaArgs);
} // fl::logger::Singleton::hexDumpF

///////////////////////////////////////////////////////////////////////
// [public]
void printF(Level level, const char* format, ...)
{
	va_list vaList;

	va_start(vaList, format);
	singleton.printF(level, format, vaList);
	va_end(vaList);
} // fl::logger::printF

///////////////////////////////////////////////////////////////////////
// [public]
void hexDumpF(Level level, const void* ptr, size_t bytes, const char* format, ...)
{
	va_list vaList;

	va_start(vaList, format);
	singleton.hexDumpF(level, ptr, bytes, format, vaList);
	va_end(vaList);
} // fl::logger::hexDumpF

///////////////////////////////////////////////////////////////////////
// [public]
void printRF(fl::Result result, Level threshold, const char* format, ...)
{
	ResultSeverity severity(RESULT_SEVERITY(result));

	if (severity < fl::ResultSeverity::COUNT) {
		Level level(fl::logger::c_severityLevel[unsigned(severity)]);

		if (severity == fl::ResultSeverity::FATAL)
			DEBUGBREAK();
		if (level <= threshold) {
			va_list vaList;

			va_start(vaList, format);
			singleton.printF(level, format, vaList);
			va_end(vaList);
		}
	}
} // fl::logger::printRF

///////////////////////////////////////////////////////////////////////
// [public]
void hexDumpRF(fl::Result result, Level threshold, const void* ptr, size_t bytes, const char* format, ...)
{
	ResultSeverity severity(RESULT_SEVERITY(result));

	if (severity < fl::ResultSeverity::COUNT) {
		Level level(fl::logger::c_severityLevel[unsigned(severity)]);

		if (level <= threshold) {
			va_list vaList;

			va_start(vaList, format);
			singleton.hexDumpF(level, ptr, bytes, format, vaList);
			va_end(vaList);
		}
	}
} // fl::logger::hexDumpRF

} // fl::logger
} // fl

#if (LOG_OUTPUT&LOG_OUTPUT_FILE)
	///////////////////////////////////////////////////////////////////////////////
	// [private, static]
	char fl::logger::Singleton::FileCycle::s_baseName[MAX_PATH] = {0};

	///////////////////////////////////////////////////////////////////////////////
	// [internal]
	fl::logger::Singleton::FileCycle::FileCycle(void)
	:   m_handle(INVALID_HANDLE_VALUE)
	,   m_size(0)
	,   m_cycleIndex(0)
	{
	#if (LOG_RANGE != LOG_RANGE_MACHINE)
		if (s_baseName[0] == '\0') {
		#if (__linux__ || ANDROID_NDK)
			strcpy(s_baseName, program_invocation_short_name);
		#else
				char processPath[MAX_PATH];
			unsigned n;

			// extract the process name
			n = GetModuleFileNameA(nullptr, processPath, MAX_PATH);
			if (n > 0) {
				char ch;
				bool hasExtension(false);
				bool hasName(false);

				do {
					n--;
					ch = processPath[n];
					switch (ch) {
					case '.':
						if (hasExtension == false) {
							// eliminate extension
							hasExtension = true;
							processPath[n] = '\0';
						}
						break;

					case '\\':
					case '/':
						n++;
						for (unsigned i = 0;  (s_baseName[i] = processPath[n + i]) != '\0';  i++)
						{
							ch = processPath[n + i];
							if (i >= sizeof(s_baseName) - 1)
								ch = '\0';
							s_baseName[i] = ch;
							if (ch == '\0')
								break;
						}
						hasName = true;
						break;
					}
				}
				while (n > 0 && hasName == false);
			}
			else
				s_baseName[0] = '\0';
		#endif
		}
		#if (LOG_RANGE == LOG_RANGE_PROCESS)
		_Open(0);
		#endif
	#else
		s_baseName[0] = '\0';
	#endif
	} // fl::logger::Singleton::FileCycle::FileCycle

	///////////////////////////////////////////////////////////////////////////////
	// [internal]
	fl::logger::Singleton::FileCycle::~FileCycle()
	{
		if (m_handle != INVALID_HANDLE_VALUE)
			Close();
	} // fl::logger::Singleton::FileCycle::~FileCycle

	///////////////////////////////////////////////////////////////////////////////
	// [private]
	fl::Result fl::logger::Singleton::FileCycle::_Open(unsigned cycleIndex)
	{
					   fl::Result result;
							 char path[MAX_PATH];
						 unsigned pathLength;
					   FileHandle handle;
	#if defined(_WIN32) ||  defined(_WIN64)
		WIN32_FILE_ATTRIBUTE_DATA fad;
							DWORD dwCreationDispostion;
	#endif

		_Close();
	#if (LOG_RANGE == LOG_RANGE_MACHINE)
		pathLength = snprintf(path, MAX_PATH, LOG_FILE_DIR LOG_FILE_BASENAME "%u.log", cycleIndex);
	#elif (LOG_RANGE == LOG_RANGE_PROCESS)
		pathLength = snprintf(path, MAX_PATH, LOG_FILE_DIR "%s.%u.log", s_baseName, cycleIndex);
	#elif (LOG_RANGE == LOG_RANGE_THREAD)
		pathLength = snprintf(path, MAX_PATH, LOG_FILE_DIR "%s.%0*X.%u.log", s_baseName, int(THREAD_ID_LENGTH), unsigned(_GetCallingThreadId()), cycleIndex);
	#endif
		if (pathLength < MAX_PATH) {
		#if (SHARED_LOG_DEBUG)
			printf("fl::logger::Singleton::FileCycle::_Open[%p](%u), path=%s\r\n", this, cycleIndex, path);
		#endif
		#if (__linux__ || ANDROID_NDK)
			handle = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR | S_IRGRP);
			if (handle > 0) {
				m_handle     = handle;
				m_size       = 0;
				m_cycleIndex = cycleIndex;
				result = fl::Result::OK;
			}
			else {
			#if (SHARED_LOG_DEBUG)
				printf("creat(%s): 0x%X\n", path, errno);
			#endif
				result = getSystemResult();
			}
		#else // Windows
			dwCreationDispostion = (GetFileAttributesExA(path, GetFileExInfoStandard, &fad)
									? TRUNCATE_EXISTING
									: OPEN_ALWAYS);

			handle = CreateFileA(path,
									GENERIC_WRITE,
									FILE_SHARE_READ | FILE_SHARE_WRITE,
									nullptr,
			#if (LOG_RANGE == LOG_RANGE_MACHINE)
									OPEN_ALWAYS,
			#else
									dwCreationDispostion,
			#endif
									FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
									nullptr);

			if (handle != INVALID_HANDLE_VALUE) {
			#if (LOG_RANGE != LOG_RANGE_MACHINE)
				if (dwCreationDispostion != TRUNCATE_EXISTING)
					SetFilePointer(handle, 0, nullptr, FILE_END);
			#endif
				m_handle     = handle;
				m_size       = 0;
				m_cycleIndex = cycleIndex;
				result = fl::Result::OK;
			}
			else {
			#if (SHARED_LOG_DEBUG)
				printf("::CreateFile(%s): 0x%X\n", path, GetLastError());
			#endif
				result = getSystemResult();
			}
		#endif
		}
		else
			result = fl::Result::E_OVERFLOW;

		return result;
	} // fl::logger::Singleton::FileCycle::_Open

	///////////////////////////////////////////////////////////////////////////////
	// [private]
	void fl::logger::Singleton::FileCycle::_Close(void)
	{
		if (m_handle != INVALID_HANDLE_VALUE) {
			Flush();
		#if (__linux__ || ANDROID_NDK)
			close(m_handle);
		#else // Windows
			CloseHandle(m_handle);
		#endif
			m_handle = INVALID_HANDLE_VALUE;
		}
	} // fl::logger::Singleton::FileCycle::Close

	///////////////////////////////////////////////////////////////////////////////
	// [internal]
	inline void fl::logger::Singleton::FileCycle::Close(void)
	{
		_Close();
		m_size       = 0;
		m_cycleIndex = 0;
	} // fl::logger::Singleton::FileCycle::Close

	///////////////////////////////////////////////////////////////////////////////
	// [internal]
	fl::Result fl::logger::Singleton::FileCycle::Write(const char* output, size_t bytes)
	{
		fl::Result result(fl::Result::OK);

		if (m_size + bytes > LOG_FILE_SIZE_LIMIT) {
			unsigned nextIndex;

			nextIndex = m_cycleIndex + 1;
			if (nextIndex >= LOG_FILE_CYCLE)
				nextIndex = 0;

			result = _Open(nextIndex);
		}
		if (m_handle != INVALID_HANDLE_VALUE) {
		#if (__linux__ || ANDROID_NDK)
			int written;

			written = write(m_handle, output, bytes);
			if (written > 0)
				m_size += written;
			else
				result = getSystemResult();
		#else // Windows
			DWORD written(0);

			if (WriteFile(m_handle, output, bytes, &written, nullptr)) {
				// this is not particularly efficient way of writting, but we hardly have an alternative
				FlushFileBuffers(m_handle);
				m_size += written;
			}
			else
				result = getSystemResult();
		#endif
		}
		return result;
	} // fl::logger::Singleton::FileCycle::Write

	///////////////////////////////////////////////////////////////////////////////
	// [internal]
	inline void fl::logger::Singleton::FileCycle::Flush(void)
	{
	#if (__linux__ || ANDROID_NDK)
		// no buffering = no flushing either <= we're using file descriptors
	#else // Windows
		SetEndOfFile(m_handle);
	#endif
	} // fl::logger::Singleton::FileCycle::Flush
#endif // (LOG_OUTPUT&LOG_OUTPUT_FILE)

#if (LOG_RANGE == LOG_RANGE_MACHINE)
	///////////////////////////////////////////////////////////////////////////////
	// [internal]
	#if (__linux__ || ANDROID_NDK)
		const char* fl::logger::Singleton::Mutex::GLOBAL_MUTEX_NAME = "/var/lock/SGPULog";
	#else // Windows
		const char* fl::logger::Singleton::Mutex::GLOBAL_MUTEX_NAME = "Global\\SGPULog";
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// [internal]
fl::logger::Singleton::Mutex::Mutex(void)
{
#if (MUTEX_RANGE == LOG_RANGE_MACHINE)
	#if (__linux__ || ANDROID_NDK)
		m_handle = open(GLOBAL_MUTEX_NAME, O_CREAT | O_NOATIME | O_RDONLY, S_IRUSR | S_IRGRP);
		#if (SHARED_LOG_DEBUG)
			if (m_handle <= 0)
				printf("FATAL: fl::logger::Mutex::Mutex(): open(%s): %d\r\n", GLOBAL_MUTEX_NAME, errno);
		#endif
	#else // Windows
		m_handle = CreateMutexA(nullptr, FALSE, GLOBAL_MUTEX_NAME);
		if (m_handle == nullptr)
			m_handle = OpenMutexA(SYNCHRONIZE, FALSE, GLOBAL_MUTEX_NAME);
		#if (SHARED_LOG_DEBUG)
			if (m_handle == nullptr)
				printf("FATAL: fl::logger::Mutex::Mutex(): OpenMutex(): 0x%X\r\n", GetLastError());
		#endif
	#endif
#endif
} // fl::logger::Singleton::Mutex::Mutex

///////////////////////////////////////////////////////////////////////////////
// [internal]
fl::logger::Singleton::Mutex::~Mutex()
{
#if (MUTEX_RANGE == LOG_RANGE_MACHINE)
	#if (__linux__ || ANDROID_NDK)
		if (m_handle != INVALID_HANDLE_VALUE) {
			close(m_handle);
			m_handle = INVALID_HANDLE_VALUE;
		}
	#else
		if (m_handle) {
			CloseHandle(m_handle);
			m_handle = nullptr;
		}
	#endif
#endif
} // fl::logger::Singleton::Mutex::~Mutex

#if (MUTEX_RANGE != LOG_RANGE_THREAD)
	///////////////////////////////////////////////////////////////////////////////
	// [internal]
	fl::Result fl::logger::Singleton::Mutex::Lock(void)
	{
	#if (MUTEX_RANGE == LOG_RANGE_MACHINE)
		#if (__linux__ || ANDROID_NDK)
			if (m_handle != INVALID_HANDLE_VALUE) {
				if (flock(m_handle, LOCK_EX) == 0)
					return fl::Result::OK;
				else
					return getSystemResult();
			}
			else {
				return fl::Result::E_INVALID_OBJECT;
			}
		#else // Windows
			if (m_handle) {
				switch (WaitForSingleObject(m_handle, INFINITE)) {
				case WAIT_OBJECT_0:
					return fl::Result::OK;
				case WAIT_ABANDONED:
					return fl::Result::E_INVALID_OBJECT;
				default:
					return getSystemResult();
				}
			}
			else
				return fl::Result::E_INVALID_OBJECT;
		#endif
	#elif (MUTEX_RANGE == LOG_RANGE_PROCESS)
		// convert back to an error code what std::mutex::lock has just converted to exception
		try {
			std::mutex::lock();
			return Result::OK;
		}
		catch (std::system_error e) {
			return MAKE_RESULT(fl::ResultSeverity::ERROR, fl::ResultOrigin::SYSTEM, e.code().value());
		}
	#endif
	} // fl::logger::Singleton::Mutex::Lock

	#if (MUTEX_RANGE == LOG_RANGE_MACHINE)
		///////////////////////////////////////////////////////////////////////////////
		// [internal]
		void fl::logger::Singleton::Mutex::Unlock(void)
		{
		#if (__linux__ || ANDROID_NDK)
			if (m_handle != INVALID_HANDLE_VALUE)
				flock(m_handle, LOCK_UN);
		#else
			if (m_handle)
				ReleaseMutex(m_handle);
		#endif
		} // fl::logger::Singleton::Mutex::Unlock
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// [internal]
fl::logger::Singleton::TimeStamp::TimeStamp(void)
{
#if (C11_TIMESTAMP || (__linux__ || ANDROID_NDK))
	Catch();
	#if (C11_TIMESTAMP)
		m_ref = m_now;
	#endif
	std::time_t systemTime = std::time(nullptr);
	std::tm* tm(std::localtime(&systemTime));
	if (tm) {
		m_timeOffset = LOG_TIME_RESOLUTION*uint64_t(unsigned(tm->tm_sec) + 60*(unsigned(tm->tm_min) + 60*unsigned(tm->tm_hour)));
	#if (!C11_TIMESTAMP)
		m_timeOffset -= m_now;
	#endif
	}
	else
		m_timeOffset = 0;
#else
	LARGE_INTEGER countsPerSecond;

	if (QueryPerformanceFrequency(&countsPerSecond) && countsPerSecond.QuadPart > 0)
	{
		SYSTEMTIME localTime;

		m_timeUnitsPerCount = double(LOG_TIME_RESOLUTION)/countsPerSecond.QuadPart;
		Catch();
		GetLocalTime(&localTime);
		m_timeOffset = (LOG_TIME_RESOLUTION/1000)*uint64_t((DWORD)localTime.wMilliseconds + 1000*((DWORD)localTime.wSecond + 60*((DWORD)localTime.wMinute + 60*(DWORD)localTime.wHour))) - m_now;
	}
	else
	{
		m_timeUnitsPerCount = 1;
		m_timeOffset = 0;
	}
#endif
} // fl::logger::Singleton::TimeStamp::TimeStamp

///////////////////////////////////////////////////////////////////////////////
// [internal]
void fl::logger::Singleton::TimeStamp::Catch(void)
{
#if (C11_TIMESTAMP)
	m_now = Clock::now();
#else
	#if (__linux__ || ANDROID_NDK)
		struct timeval tv;

		if (gettimeofday(&tv, nullptr) == 0) {
			m_now = static_cast<TimePoint>(tv.tv_sec)*LOG_TIME_RESOLUTION
		#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_US)
					+ static_cast<TimePoint>(tv.tv_usec)*(LOG_TIME_RESOLUTION/1000000);
		#else
					+ static_cast<TimePoint>(tv.tv_usec)/1000;
		#endif
		}
	#else // Windows
		LARGE_INTEGER counts;

		if (QueryPerformanceCounter(&counts))
			m_now = static_cast<TimePoint>(m_timeUnitsPerCount*counts.QuadPart);
	#endif
		else
			m_now = 0;
#endif
} // fl::logger::Singleton::TimeStamp::Catch

///////////////////////////////////////////////////////////////////////////////
// [internal]
unsigned fl::logger::Singleton::TimeStamp::Format(char* output)
{
	TimePoint t;    // time in subsequent units
	TimePoint tr;   // 64b remainder
	 unsigned ur,   // 32b remainder
			  h,    // hours
			  m,    // minutes
			  s,    // seconds
			  ms;   // milliseconds
#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_US)
	TimePoint  us;   // microseconds
#endif
#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_NS)
	TimePoint  ns;   // microseconds
#endif

#if (C11_TIMESTAMP)
	#if (LOG_TIME_RESOLUTION == LOG_TIME_RESOLUTION_MS)
		t = m_timeOffset + std::chrono::duration_cast<std::chrono::milliseconds>(m_now - m_ref).count();
	#elif (LOG_TIME_RESOLUTION == LOG_TIME_RESOLUTION_US)
		t = m_timeOffset + std::chrono::duration_cast<std::chrono::microseconds>(m_now - m_ref).count();
	#elif (LOG_TIME_RESOLUTION == LOG_TIME_RESOLUTION_NS)
		t = m_timeOffset + std::chrono::duration_cast<std::chrono::nanoseconds> (m_now - m_ref).count();
	#else
		#error Unsupported LOG_TIME_RESOLUTION
	#endif
#else
	t  = m_timeOffset + m_now;
#endif
	// split into {h, m, s, ms, us, ns}
	h  = 0;
	m  = 0;
	s  = 0;
#if (LOG_TIME_RESOLUTION > LOG_TIME_RESOLUTION_MS)
	ms = 0;
#endif
#if (LOG_TIME_RESOLUTION > LOG_TIME_RESOLUTION_US)
	us = 0;
#endif
#if (LOG_TIME_RESOLUTION > LOG_TIME_RESOLUTION_NS)
	ns = 0;
#endif
	tr = t/1000;
#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_NS)
	ns = static_cast<unsigned>(t - 1000*tr);
	if (tr) {
		t = tr;
		tr /= 1000;
#endif
	#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_US)
		us = static_cast<unsigned>(t - 1000*tr);
		if (tr) {
			t = tr;
			tr /= 1000;
	#endif
			ms = static_cast<unsigned>(t - 1000*tr);
			if (tr) {
				t = tr;
				tr /= 60;
				s = static_cast<unsigned>(t - 60*tr);
				if (tr) {
					t = tr;
					tr /= 60;
					m = static_cast<unsigned>(t - 60*tr);
					if (tr) {
						t = tr;
						tr /= 24;
						h = static_cast<unsigned>(t - 24*tr);
					}
				}
			}
	#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_US)
		}
	#endif
#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_NS)
	}
#endif
	// inlined snprintf(output, TIMESTAMP_LENGTH, "%02u:%02u:%02u.%03u.%03u", h, m, s, ms, us):
	output[ 0] = '0' + char(ur = h/10);
	output[ 1] = '0' + char(h - 10*ur);
	output[ 2] = ':';
	output[ 3] = '0' + char(ur = m/10);
	output[ 4] = '0' + char(m - 10*ur);
	output[ 5] = ':';
	output[ 6] = '0' + char(ur = s/10);
	output[ 7] = '0' + char(s - 10*ur);
	output[ 8] = '.';
	output[ 9] = '0' + char(ur = ms/100); ms -= 100*ur;
	output[10] = '0' + char(ur = ms/10);  ms -=  10*ur;
	output[11] = '0' + char     (ms);
#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_US)
	output[12] = '.';
	output[13] = '0' + char(ur = unsigned(us/100)); us -= 100*ur;
	output[14] = '0' + char(ur = unsigned(us/10));  us -=  10*ur;
	output[15] = '0' + char              (us);
#endif
#if (LOG_TIME_RESOLUTION >= LOG_TIME_RESOLUTION_NS)
	output[16] = '.';
	output[17] = '0' + char(ur = unsigned(ns/100)); ns -= 100*ur;
	output[18] = '0' + char(ur = unsigned(ns/10));  ns -=  10*ur;
	output[19] = '0' + char              (ns);
#endif
	return TIMESTAMP_LENGTH;
} // fl::logger::Singleton::TimeStamp::Format
