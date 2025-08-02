/*
 * Copyright (c) 2002-2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __mDNSDebug_h
#define __mDNSDebug_h

#include "mDNSFeatures.h"

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
#include <os/log.h>
#include "mDNSDebugShared.h"
#endif

// Set MDNS_DEBUGMSGS to 0 to optimize debugf() calls out of the compiled code
// Set MDNS_DEBUGMSGS to 1 to generate normal debugging messages
// Set MDNS_DEBUGMSGS to 2 to generate verbose debugging messages
// MDNS_DEBUGMSGS is normally set in the project options (or makefile) but can also be set here if desired
// (If you edit the file here to turn on MDNS_DEBUGMSGS while you're debugging some code, be careful
// not to accidentally check-in that change by mistake when you check in your other changes.)

#ifndef MDNS_DEBUGMSGS
#define MDNS_DEBUGMSGS 0
#endif

// Set MDNS_CHECK_PRINTF_STYLE_FUNCTIONS to 1 to enable extra GCC compiler warnings
// Note: You don't normally want to do this, because it generates a bunch of
// spurious warnings for the following custom extensions implemented by mDNS_vsnprintf:
//    warning: `#' flag used with `%s' printf format    (for %#s              -- pascal string format)
//    warning: repeated `#' flag in format              (for %##s             -- DNS name string format)
//    warning: double format, pointer arg (arg 2)       (for %.4a, %.16a, %#a -- IP address formats)
#ifndef MDNS_CHECK_PRINTF_STYLE_FUNCTIONS
#define MDNS_CHECK_PRINTF_STYLE_FUNCTIONS 0
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
typedef os_log_t mDNSLogCategory_t;

typedef os_log_type_t mDNSLogLevel_t;
#define MDNS_LOG_FAULT      OS_LOG_TYPE_FAULT
#define MDNS_LOG_ERROR      OS_LOG_TYPE_ERROR
#define MDNS_LOG_WARNING    OS_LOG_TYPE_DEFAULT
#define MDNS_LOG_DEFAULT    OS_LOG_TYPE_DEFAULT
#define MDNS_LOG_INFO       OS_LOG_TYPE_INFO
#define MDNS_LOG_DEBUG      OS_LOG_TYPE_DEBUG
#else
typedef const char * mDNSLogCategory_t;
typedef enum
{
    MDNS_LOG_FAULT   = 1,
    MDNS_LOG_ERROR   = 2,
    MDNS_LOG_WARNING = 3,
    MDNS_LOG_DEFAULT = 4,
    MDNS_LOG_INFO    = 5,
    MDNS_LOG_DEBUG   = 6
} mDNSLogLevel_t;
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)

    #define MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(NAME)                       \
        extern os_log_t mDNSLogCategory_ ## NAME;                           \
        extern os_log_t mDNSLogCategory_ ## NAME ## _redacted

    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(Default);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(State);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(mDNS);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(uDNS);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(SPS);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(NAT);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(D2D);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(XPC);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(Analytics);
    MDNS_OS_LOG_CATEGORY_DECLARE_EXTERN(DNSSEC);

    #define MDNS_LOG_CATEGORY_DEFINITION(NAME)  mDNSLogCategory_ ## NAME
#else
    #define MDNS_LOG_CATEGORY_DEFINITION(NAME)  # NAME
#endif

#define MDNS_LOG_CATEGORY_DEFAULT   MDNS_LOG_CATEGORY_DEFINITION(Default)
#define MDNS_LOG_CATEGORY_STATE     MDNS_LOG_CATEGORY_DEFINITION(State)
#define MDNS_LOG_CATEGORY_MDNS      MDNS_LOG_CATEGORY_DEFINITION(mDNS)
#define MDNS_LOG_CATEGORY_UDNS      MDNS_LOG_CATEGORY_DEFINITION(uDNS)
#define MDNS_LOG_CATEGORY_SPS       MDNS_LOG_CATEGORY_DEFINITION(SPS)
#define MDNS_LOG_CATEGORY_NAT       MDNS_LOG_CATEGORY_DEFINITION(NAT)
#define MDNS_LOG_CATEGORY_D2D       MDNS_LOG_CATEGORY_DEFINITION(D2D)
#define MDNS_LOG_CATEGORY_XPC       MDNS_LOG_CATEGORY_DEFINITION(XPC)
#define MDNS_LOG_CATEGORY_ANALYTICS MDNS_LOG_CATEGORY_DEFINITION(Analytics)
#define MDNS_LOG_CATEGORY_DNSSEC    MDNS_LOG_CATEGORY_DEFINITION(DNSSEC)

// Use MDNS_LOG_CATEGORY_DISABLED to disable a log temporarily.
#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define MDNS_LOG_CATEGORY_DISABLED OS_LOG_DISABLED
#else
    #define MDNS_LOG_CATEGORY_DISABLED "Log Disabled"
#endif

// Set this symbol to 1 to answer remote queries for our Address, and reverse mapping PTR
#define ANSWER_REMOTE_HOSTNAME_QUERIES 0

// Set this symbol to 1 to do extra debug checks on malloc() and free()
// Set this symbol to 2 to write a log message for every malloc() and free()
#ifndef MDNS_MALLOC_DEBUGGING
#define MDNS_MALLOC_DEBUGGING 0
#endif

#if (MDNS_MALLOC_DEBUGGING > 0) && defined(WIN32)
#error "Malloc debugging does not yet work on Windows"
#endif

#define ForceAlerts 0
//#define LogTimeStamps 1

// Developer-settings section ends here

#if MDNS_CHECK_PRINTF_STYLE_FUNCTIONS
#define IS_A_PRINTF_STYLE_FUNCTION(F,A) __attribute__ ((format(printf,F,A)))
#else
#define IS_A_PRINTF_STYLE_FUNCTION(F,A)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Variable argument macro support. Use ANSI C99 __VA_ARGS__ where possible. Otherwise, use the next best thing.

#if (defined(__GNUC__))
    #if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 2)))
        #define MDNS_C99_VA_ARGS        1
        #define MDNS_GNU_VA_ARGS        0
    #else
        #define MDNS_C99_VA_ARGS        0
        #define MDNS_GNU_VA_ARGS        1
    #endif
    #define MDNS_HAS_VA_ARG_MACROS      1
#elif (_MSC_VER >= 1400) // Visual Studio 2005 and later
    #define MDNS_C99_VA_ARGS            1
    #define MDNS_GNU_VA_ARGS            0
    #define MDNS_HAS_VA_ARG_MACROS      1
#elif (defined(__MWERKS__))
    #define MDNS_C99_VA_ARGS            1
    #define MDNS_GNU_VA_ARGS            0
    #define MDNS_HAS_VA_ARG_MACROS      1
#else
    #define MDNS_C99_VA_ARGS            0
    #define MDNS_GNU_VA_ARGS            0
    #define MDNS_HAS_VA_ARG_MACROS      0
#endif

#if (MDNS_HAS_VA_ARG_MACROS)
    #if (MDNS_C99_VA_ARGS)
        #define MDNS_LOG_DEFINITION(LEVEL, ...) \
            do { if (mDNS_LoggingEnabled) LogMsgWithLevel(MDNS_LOG_CATEGORY_DEFAULT, LEVEL, __VA_ARGS__); } while (0)

        #define debug_noop(...)   do {} while(0)
        #define LogMsg(...)       LogMsgWithLevel(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, __VA_ARGS__)
        #define LogOperation(...) MDNS_LOG_DEFINITION(MDNS_LOG_DEFAULT,  __VA_ARGS__)
        #define LogSPS(...)       MDNS_LOG_DEFINITION(MDNS_LOG_DEFAULT,  __VA_ARGS__)
        #define LogInfo(...)      MDNS_LOG_DEFINITION(MDNS_LOG_DEFAULT,  __VA_ARGS__)
        #define LogDebug(...)     MDNS_LOG_DEFINITION(MDNS_LOG_DEBUG, __VA_ARGS__)
    #elif (MDNS_GNU_VA_ARGS)
        #define MDNS_LOG_DEFINITION(LEVEL, ARGS...) \
            do { if (mDNS_LoggingEnabled) LogMsgWithLevel(MDNS_LOG_CATEGORY_DEFAULT, LEVEL, ARGS); } while (0)

        #define debug_noop(ARGS...)   do {} while (0)
        #define LogMsg(ARGS... )      LogMsgWithLevel(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, ARGS)
        #define LogOperation(ARGS...) MDNS_LOG_DEFINITION(MDNS_LOG_DEFAULT,  ARGS)
        #define LogSPS(ARGS...)       MDNS_LOG_DEFINITION(MDNS_LOG_DEFAULT,  ARGS)
        #define LogInfo(ARGS...)      MDNS_LOG_DEFINITION(MDNS_LOG_DEFAULT,  ARGS)
        #define LogDebug(ARGS...)     MDNS_LOG_DEFINITION(MDNS_LOG_DEBUG, ARGS)
    #else
        #error "Unknown variadic macros"
    #endif
#else
// If your platform does not support variadic macros, you need to define the following variadic functions.
// See mDNSShared/mDNSDebug.c for sample implementation
    #define debug_noop 1 ? (void)0 : (void)
    #define LogMsg LogMsg_
    #define LogOperation (mDNS_LoggingEnabled == 0) ? ((void)0) : LogOperation_
    #define LogSPS       (mDNS_LoggingEnabled == 0) ? ((void)0) : LogSPS_
    #define LogInfo      (mDNS_LoggingEnabled == 0) ? ((void)0) : LogInfo_
    #define LogDebug     (mDNS_LoggingEnabled == 0) ? ((void)0) : LogDebug_
extern void LogMsg_(const char *format, ...)       IS_A_PRINTF_STYLE_FUNCTION(1,2);
extern void LogOperation_(const char *format, ...) IS_A_PRINTF_STYLE_FUNCTION(1,2);
extern void LogSPS_(const char *format, ...)       IS_A_PRINTF_STYLE_FUNCTION(1,2);
extern void LogInfo_(const char *format, ...)      IS_A_PRINTF_STYLE_FUNCTION(1,2);
extern void LogDebug_(const char *format, ...)     IS_A_PRINTF_STYLE_FUNCTION(1,2);
#endif


#if MDNS_DEBUGMSGS
#define debugf debugf_
extern void debugf_(const char *format, ...) IS_A_PRINTF_STYLE_FUNCTION(1,2);
#else
#define debugf debug_noop
#endif

#if MDNS_DEBUGMSGS > 1
#define verbosedebugf verbosedebugf_
extern void verbosedebugf_(const char *format, ...) IS_A_PRINTF_STYLE_FUNCTION(1,2);
#else
#define verbosedebugf debug_noop
#endif

extern int mDNS_LoggingEnabled;
extern int mDNS_DebugLoggingEnabled;
extern int mDNS_PacketLoggingEnabled;
extern int mDNS_McastLoggingEnabled;
extern int mDNS_McastTracingEnabled;
extern int mDNS_DebugMode;          // If non-zero, LogMsg() writes to stderr instead of syslog

#if MDNSRESPONDER_SUPPORTS(APPLE, LOG_PRIVACY_LEVEL)
extern int gNumOfSensitiveLoggingEnabledQuestions;
extern int gSensitiveLoggingEnabled; // If true, LogRedact() will redact all private level logs. The content of state
                                        // dump that is related to user's privacy will also be redacted.
#endif

extern const char ProgramName[];

extern void LogMsgWithLevel(mDNSLogCategory_t category, mDNSLogLevel_t level, const char *format, ...) IS_A_PRINTF_STYLE_FUNCTION(3,4);
// LogMsgNoIdent needs to be fixed so that it logs without the ident prefix like it used to
// (or completely overhauled to use the new "log to a separate file" facility)
#define LogMsgNoIdent LogMsg

#define LogFatalError LogMsg

#if MDNS_MALLOC_DEBUGGING >= 1
extern void *mallocL(const char *msg, mDNSu32 size);
extern void *callocL(const char *msg, mDNSu32 size);
extern void freeL(const char *msg, void *x);
#define LogMemCorruption LogMsg
#else
#define mallocL(MSG, SIZE) mdns_malloc(SIZE)
#define callocL(MSG, SIZE) mdns_calloc(1, SIZE)
#define freeL(MSG, PTR) mdns_free(PTR)
#endif

#ifdef __cplusplus
}
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)

/** @brief Write a log message to system's log storage(memory or disk).
 *
 *  On Apple platform, os_log() will be called to log a message.
 *
 *  @param CATEGORY         A custom log object previously created by the os_log_create function, and such an object is
 *                          used to specify "subsystem" and "category". For mDNSResponder, the subsystem should always
 *                          be set to "com.apple.mDNSResponder"; and the category is used for categorization and
 *                          filtering of related log messages within the subsystem’s settings. We have 4 categories that
 *                          are pre-defined: MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_CATEGORY_MDNS, MDNS_LOG_CATEGORY_UDNS,
 *                          MDNS_LOG_CATEGORY_SPS. If these categories are not enough, use os_log_create to create more.
 *
 *  @param LEVEL            The log level that determines the importance of the message. The levels are, in order of
 *                          decreasing importance:
 *                              MDNS_LOG_FAULT      Fault-level messages are intended for capturing system-level errors
 *                                                  that are critical to the system. They are always saved in the data store.
 *                              MDNS_LOG_ERROR      Error-level messages are intended for reporting process-level errors
 *                                                  that are unexpected and incorrect during the normal operation. They
 *                                                  are always saved in the data store.
 *                              MDNS_LOG_WARNING    Warning-level messages are intended for capturing unexpected and
 *                                                  possible incorrect behavior that might be used later to root cause
 *                                                  an error or fault. They are are initially stored in memory buffers
 *                                                  and then moved to a data store.
 *                              MDNS_LOG_DEFAULT    Default-level messages are intended for reporting things that might
 *                                                  result a failure. They are are initially stored in memory buffers
 *                                                  and then moved to a data store.
 *                              MDNS_LOG_INFO       Info-level messages are intended for capturing information that may
 *                                                  be helpful, but isn’t essential, for troubleshooting errors. They
 *                                                  are initially stored in memory buffers, but will only be moved into
 *                                                  data store when faults and, optionally, errors occur.
 *                              MDNS_LOG_DEBUG      Debug-level messages are intended for information that may be useful
 *                                                  during development or while troubleshooting a specific problem, Debug
 *                                                  logging should not be used in shipping software. They are only
 *                                                  captured in memory when debug logging is enabled through a
 *                                                  configuration change.
 *
 *  @param FORMAT           A constant string or format string that produces a human-readable log message. The format
 *                          string follows the IEEE printf specification, besides the following customized format specifiers:
 *                              %{mdnsresponder:domain_name}.*P     the pointer to a DNS lable sequence
 *                              %{mdnsresponder:ip_addr}.20P        the pointer to a mDNSAddr variable
 *                              %{network:in_addr}.4P               the pointer to a mDNSv4Addr variable
 *                              %{network:in6_addr}.16P             the pointer to a mDNSv6Addr variable
 *                              %{mdnsresponder:mac_addr}.6P        the pointer to a 6-byte-length MAC address
 *
 *  @param ...              The parameter list that will be formated by the format string. Note that if the customized
 *                          format specifiers are used and the data length is not specified in the format string, the
 *                          size should be listed before the pointer to the data, for example:
 *                              "%{mdnsresponder:domain_name}.*P", (name ? (int)DomainNameLength((const domainname *)name) : 0), <the pointer to a DNS label sequence>
 *
 */
    #define JOIN(X, Y) JOIN_AGAIN(X, Y)
    #define JOIN_AGAIN(X, Y) X ## Y

    #define LogRedact(CATEGORY, LEVEL, FORMAT, ...)                                         \
        do                                                                                  \
        {                                                                                   \
            if (!gSensitiveLoggingEnabled || ((CATEGORY) == (MDNS_LOG_CATEGORY_STATE)))     \
            {                                                                               \
                os_log_with_type(CATEGORY, LEVEL, FORMAT, ## __VA_ARGS__);                  \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                os_log_with_type(JOIN(CATEGORY, _redacted), LEVEL, FORMAT, ## __VA_ARGS__); \
            }                                                                               \
        } while(0)
#else
    #if (MDNS_HAS_VA_ARG_MACROS)
        #if (MDNS_C99_VA_ARGS)
            #define LogRedact(CATEGORY, LEVEL, ...) \
                do { if (mDNS_LoggingEnabled) LogMsgWithLevel(CATEGORY, LEVEL, __VA_ARGS__); } while (0)
        #elif (MDNS_GNU_VA_ARGS)
            #define LogRedact(CATEGORY, LEVEL, ARGS...) \
                do { if (mDNS_LoggingEnabled) LogMsgWithLevel(CATEGORY, LEVEL, ARGS); } while (0)
        #else
            #error "Unknown variadic macros"
        #endif
    #else
        #define LogRedact      (mDNS_LoggingEnabled == 0) ? ((void)0) : LogRedact_
        extern void LogRedact_(const char *format, ...) IS_A_PRINTF_STYLE_FUNCTION(1,2);
    #endif
#endif // MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)

//======================================================================================================================
// MARK: - RData Log Helper

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define MDNS_CORE_LOG_RDATA_WITH_BUFFER(CATEGORY, LEVEL, RR_PTR, RDATA_BUF, RDATA_BUF_LEN, FORMAT, ...)     \
        do                                                                                                      \
        {                                                                                                       \
            mStatus _get_rdata_err;                                                                             \
            mDNSu16 _rdataLen;                                                                                  \
            const mDNSu8 *const _rdataBytes = ResourceRecordGetRDataBytesPointer((RR_PTR), (RDATA_BUF),         \
                (RDATA_BUF_LEN), &_rdataLen, &_get_rdata_err);                                                  \
            if (!_get_rdata_err)                                                                                \
            {                                                                                                   \
                mDNSu8 *_typeRDataBuf = mDNSNULL;                                                               \
                mDNSu32 _typeRDataLen = 0;                                                                      \
                mDNSu8 *_typeRDataBufHeap = mDNSNULL;                                                           \
                if (sizeof(mDNSStorage.MsgBuffer) >= 2 + _rdataLen)                                             \
                {                                                                                               \
                    _typeRDataBuf = (mDNSu8 *)mDNSStorage.MsgBuffer;                                            \
                    _typeRDataLen = sizeof(mDNSStorage.MsgBuffer);                                              \
                }                                                                                               \
                else                                                                                            \
                {                                                                                               \
                    _typeRDataLen = 2 + _rdataLen;                                                              \
                    _typeRDataBufHeap = mDNSPlatformMemAllocate(_typeRDataLen);                                 \
                    _typeRDataBuf = _typeRDataBufHeap;                                                          \
                }                                                                                               \
                LogRedact(CATEGORY, LEVEL,                                                                      \
                    FORMAT "type: " PUB_DNS_TYPE ", rdata: " PRI_RDATA,                                         \
                    ##__VA_ARGS__, DNS_TYPE_PARAM((RR_PTR)->rrtype), RDATA_PARAM(_typeRDataBuf, _typeRDataLen,  \
                    (RR_PTR)->rrtype, _rdataBytes, _rdataLen));                                                 \
                mDNSPlatformMemFree(_typeRDataBufHeap);                                                         \
            }                                                                                                   \
        } while (0)
#else
    #define MDNS_CORE_LOG_RDATA_WITH_BUFFER(CATEGORY, LEVEL, RR_PTR, RDATA_BUF, RDATA_BUF_LEN, FORMAT, ...)         \
        do                                                                                                          \
        {                                                                                                           \
            (void)(RDATA_BUF);                                                                                      \
            (void)(RDATA_BUF_LEN);                                                                                  \
            LogRedact(CATEGORY, LEVEL, FORMAT " " PRI_S, ##__VA_ARGS__, RRDisplayString(&mDNSStorage, (RR_PTR)));   \
        } while (0)
#endif

#define MDNS_CORE_LOG_RDATA(CATEGORY, LEVEL, RR_PTR, FORMAT, ...)                                                   \
    do                                                                                                              \
    {                                                                                                               \
        mDNSu8 *_rdataBuffer = NULL;                                                                                \
        mDNSu8 *_rdataBufferHeap = NULL;                                                                            \
        mDNSu16 _rdataBufferLen;                                                                                    \
        if ((RR_PTR)->rdlength <= sizeof(mDNSStorage.RDataBuffer))                                                  \
        {                                                                                                           \
            _rdataBuffer = mDNSStorage.RDataBuffer;                                                                 \
            _rdataBufferLen = sizeof(mDNSStorage.RDataBuffer);                                                      \
        }                                                                                                           \
        else                                                                                                        \
        {                                                                                                           \
            _rdataBufferHeap = mDNSPlatformMemAllocate((RR_PTR)->rdlength);                                         \
            _rdataBuffer = _rdataBufferHeap;                                                                        \
            _rdataBufferLen = (RR_PTR)->rdlength;                                                                   \
        }                                                                                                           \
        if ((RR_PTR)->rdlength == 0)                                                                                \
        {                                                                                                           \
            LogRedact(CATEGORY, LEVEL,                                                                              \
                FORMAT "type: " PUB_DNS_TYPE ", rdata: <none>", ##__VA_ARGS__, DNS_TYPE_PARAM((RR_PTR)->rrtype));   \
        }                                                                                                           \
        else                                                                                                        \
        {                                                                                                           \
            MDNS_CORE_LOG_RDATA_WITH_BUFFER(CATEGORY, LEVEL, RR_PTR, _rdataBuffer, _rdataBufferLen, FORMAT,         \
                ##__VA_ARGS__);                                                                                     \
        }                                                                                                           \
        mDNSPlatformMemFree(_rdataBufferHeap);                                                                      \
    }                                                                                                               \
    while(0)

//======================================================================================================================
// MARK: - Customized Log Specifier

// The followings are the customized log specifier defined in os_log. For compatibility, we have to define it when it is
// not on the Apple platform, for example, the Posix platform. The keyword "public" or "private" is used to control whether
// the content would be redacted when the redaction is turned on: "public" means the content will always be printed;
// "private" means the content will be printed as <mask.hash: '<The hashed string from binary data>'> if the redaction is turned on,
// only when the redaction is turned off, the content will be printed as what it should be. Note that the hash performed
// to the data is a salted hashing transformation, and the salt is generated randomly on a per-process basis, meaning
// that hashes cannot be correlated across processes or devices.

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PRI_PREFIX "{sensitive, mask.hash}"
#else
    #define PRI_PREFIX
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_S "%{public}s"
    #define PRI_S "%{sensitive, mask.hash}s"
#else
    #define PUB_S "%s"
    #define PRI_S PUB_S
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_BOOL                    "%{mdns:yesno}d"
    #define BOOL_PARAM(boolean_value)   (boolean_value)
#else
    #define PUB_BOOL                    PUB_S
    #define BOOL_PARAM(boolean_value)   ((boolean_value) ? "yes" : "no")
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_TIMEV                   "%{public, timeval}.*P"
    #define TIMEV_PARAM(time_val_ptr)   ((int)sizeof(*time_val_ptr)), time_val_ptr
#else
    #define PUB_TIMEV                   "%ld"
    #define TIMEV_PARAM(time_val_ptr)   ((time_val_ptr)->tv_sec)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define DNS_MSG_ID_FLAGS                                "%{mdns:dns.idflags}08lX"
    #define DNS_MSG_ID_FLAGS_PARAM(HEADER, ID_AND_FLAGS)    ((unsigned long)(ID_AND_FLAGS))
#else
    #define DNS_MSG_ID_FLAGS                                "id: 0x%04X (%u), flags: 0x%04X"
    #define DNS_MSG_ID_FLAGS_PARAM(HEADER, ID_AND_FLAGS)    mDNSVal16((HEADER).id), mDNSVal16((HEADER).id), \
                                                                ((HEADER).flags.b)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define DNS_MSG_COUNTS                          "%{mdns:dns.counts}016llX"
    #define DNS_MSG_COUNTS_PARAM(HEADER, COUNTS)    ((unsigned long long)(COUNTS))
#else
    #define DNS_MSG_COUNTS                          "counts: %u/%u/%u/%u"
    #define DNS_MSG_COUNTS_PARAM(HEADER, COUNTS)    (HEADER).numQuestions, (HEADER).numAnswers, \
                                                        (HEADER).numAuthorities, (HEADER).numAdditionals
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define MDNS_NAME_HASH_TYPE_BYTES \
        "%{sensitive, mask.hash, mdnsresponder:mdns_name_hash_type_bytes}.*P"
    #define MDNS_NAME_HASH_TYPE_BYTES_PARAM(BYTES, BYTES_LEN) BYTES_LEN, BYTES
#else
    // If os_log is not supported, there is no way to parse the name hash type bytes.
    #define MDNS_NAME_HASH_TYPE_BYTES                           "%s"
    #define MDNS_NAME_HASH_TYPE_BYTES_PARAM(BYTES, BYTES_LEN)   ""
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_DNS_TYPE                "%{mdns:rrtype}d"
    #define DNS_TYPE_PARAM(type_value)  (type_value)
#else
    #define PUB_DNS_TYPE                PUB_S
    #define DNS_TYPE_PARAM(type_value)  (DNSTypeName(type_value))
#endif

// Notes about using RMV rather than REMOVE:
// Both "add" and "rmv" are three characters so that when the log is printed, the content will be aligned which is good
// for log searching. For example:
// DNSServiceBrowse(_test._tcp.local., PTR) RESULT ADD interface 1:   23 _test._tcp.local. PTR demo._test._tcp.local.
// DNSServiceBrowse(_test._tcp.local., PTR) RESULT RMV interface 1:   23 _test._tcp.local. PTR demo._test._tcp.local.
// is better than:
// DNSServiceBrowse(_test._tcp.local., PTR) RESULT ADD interface 1:   23 _test._tcp.local. PTR demo._test._tcp.local.
// DNSServiceBrowse(_test._tcp.local., PTR) RESULT REMOVE interface 1:   23 _test._tcp.local. PTR demo._test._tcp.local.
#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_ADD_RMV                     "%{mdns:addrmv}d"
    #define ADD_RMV_PARAM(add_rmv_value)    (add_rmv_value)
#else
    #define PUB_ADD_RMV                     PUB_S
    #define ADD_RMV_PARAM(add_rmv_value)    ((add_rmv_value) ? "add" : "rmv")
#endif

// Here we have the uppercase style so that it can be used to match the original mDNSResponder RESULT ADD/RMV all
// uppercase.
#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_ADD_RMV_U                   "%{mdns:addrmv_upper}d"
    #define ADD_RMV_U_PARAM(add_rmv_value)  (add_rmv_value)
#else
    #define PUB_ADD_RMV_U                   PUB_S
    #define ADD_RMV_U_PARAM(add_rmv_value)  ((add_rmv_value) ? "ADD" : "RMV")
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_PN                      "%{mdns:pos/neg}d"
    #define PN_PARAM(pn_boolean_value)  (pn_boolean_value)
#else
    #define PUB_PN                      PUB_S
    #define PN_PARAM(pn_boolean_value)  ((pn_boolean_value) ? "positive" : "negative")
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_MORTALITY                       "%{mdns:mortality}d"
    #define MORTALITY_PARAM(mortality_value)    (mortality_value)
#else
    #define PUB_MORTALITY                       PUB_S
    #define MORTALITY_PARAM(mortality_value)    (MortalityDisplayString(mortality_value))
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_DM_NAME                 "%{public, mdnsresponder:domain_name}.*P"
    #define PRI_DM_NAME                 "%{sensitive, mask.hash, mdnsresponder:domain_name}.*P"
    // When DM_NAME_PARAM is used, the file where the function is defined must include DNSEmbeddedAPI.h
    #define DM_NAME_PARAM(name)         ((name) ? ((int)DomainNameLength((name))) : 0), (name)
    #define DM_NAME_PARAM_NONNULL(name) (int)DomainNameLength(name), (name)
#else
    #define PUB_DM_NAME                 "%##s"
    #define PRI_DM_NAME                 PUB_DM_NAME
    #define DM_NAME_PARAM(name)         (name)
    #define DM_NAME_PARAM_NONNULL(name) (name)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_DM_LABEL                "%{public, mdnsresponder:domain_label}.*P"
    #define PRI_DM_LABEL                "%{sensitive, mask.hash, mdnsresponder:domain_label}.*P"
    #define DM_LABEL_PARAM(label)       1 + ((label)->c[0]), ((label)->c)
    #define DM_LABEL_PARAM_SAFE(label)  (label ? 1 + ((label)->c[0]) : 0), ((label)->c)
#else
    #define PUB_DM_LABEL                "%#s"
    #define PRI_DM_LABEL                PUB_DM_LABEL
    #define DM_LABEL_PARAM(label)       (label)
    #define DM_LABEL_PARAM_SAFE(label)  (label)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_IP_ADDR "%{public, mdnsresponder:ip_addr}.20P"
    #define PRI_IP_ADDR "%{sensitive, mask.hash, mdnsresponder:ip_addr}.20P"

    #define PUB_IPv4_ADDR "%{public, network:in_addr}.4P"
    #define PRI_IPv4_ADDR "%{sensitive, mask.hash, network:in_addr}.4P"

    #define PUB_IPv6_ADDR "%{public, network:in6_addr}.16P"
    #define PRI_IPv6_ADDR "%{sensitive, mask.hash, network:in6_addr}.16P"
#else
    #define PUB_IP_ADDR "%#a"
    #define PRI_IP_ADDR PUB_IP_ADDR

    #define PUB_IPv4_ADDR "%.4a"
    #define PRI_IPv4_ADDR PUB_IPv4_ADDR

    #define PUB_IPv6_ADDR "%.16a"
    #define PRI_IPv6_ADDR PUB_IPv6_ADDR
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_MAC_ADDR "%{public, mdnsresponder:mac_addr}.6P"
    #define PRI_MAC_ADDR "%{sensitive, mask.hash, mdnsresponder:mac_addr}.6P"
#else
    #define PUB_MAC_ADDR "%.6a"
    #define PRI_MAC_ADDR PUB_MAC_ADDR
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_HEX "%{public, mdnsresponder:hex_sequence}.*P"
    #define PRI_HEX "%{sensitive, mask.hash, mdnsresponder:hex_sequence}.*P"
    #define HEX_PARAM(hex, hex_length) (int)(hex_length), (hex)
#else
    #define PUB_HEX "%p"
    #define PRI_HEX PUB_HEX
    #define HEX_PARAM(hex, hex_length) (hex)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_DNSKEY "%{public, mdns:rd.dnskey}.*P"
    #define PRI_DNSKEY "%{sensitive, mask.hash, mdns:rd.dnskey}.*P"
    #define DNSKEY_PARAM(rdata, rdata_length) (rdata_length), (rdata)
#else
    #define PUB_DNSKEY "%p"
    #define PRI_DNSKEY PUB_DNSKEY
    #define DNSKEY_PARAM(rdata, rdata_length) (rdata)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_DS "%{public, mdns:rd.ds}.*P"
    #define PRI_DS "%{sensitive, mask.hash, mdns:rd.ds}.*P"
    #define DS_PARAM(rdata, rdata_length) (rdata_length), (rdata)
#else
    #define PUB_DS "%p"
    #define PRI_DS PUB_DS
    #define DS_PARAM(rdata, rdata_length) (rdata)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_NSEC "%{public, mdns:rd.nsec}.*P"
    #define PRI_NSEC "%{sensitive, mask.hash, mdns:rd.nsec}.*P"
    #define NSEC_PARAM(rdata, rdata_length) (rdata_length), (rdata)
#else
    #define PUB_NSEC "%p"
    #define PRI_NSEC PUB_NSEC
    #define NSEC_PARAM(rdata, rdata_length) (rdata)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_NSEC3 "%{public, mdns:rd.nsec3}.*P"
    #define PRI_NSEC3 "%{sensitive, mask.hash, mdns:rd.nsec3}.*P"
    #define NSEC3_PARAM(rdata, rdata_length) (rdata_length), (rdata)
#else
    #define PUB_NSEC3 "%p"
    #define PRI_NSEC3 PUB_NSEC3
    #define NSEC3_PARAM(rdata, rdata_length) (rdata)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_RRSIG "%{public, mdns:rd.rrsig}.*P"
    #define PRI_RRSIG "%{sensitive, mask.hash, mdns:rd.rrsig}.*P"
    #define RRSIG_PARAM(rdata, rdata_length) (rdata_length), (rdata)
#else
    #define PUB_RRSIG "%p"
    #define PRI_RRSIG PUB_RRSIG
    #define RRSIG_PARAM(rdata, rdata_length) (rdata)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_SVCB "%{public, mdns:rd.svcb}.*P"
    #define PRI_SVCB "%{sensitive, mask.hash, mdns:rd.svcb}.*P"
    #define SVCB_PARAM(rdata, rdata_length) (rdata_length), (rdata)
#else
    #define PUB_SVCB "%p"
    #define PRI_SVCB PUB_SVCB
    #define SVCB_PARAM(rdata, rdata_length) (rdata)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    #if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
        #define PUB_DNSSEC_RESULT                           "%{public, mdns:dnssec_result}d"
        #define DNSSEC_RESULT_PARAM(dnssec_result_value)    (dnssec_result_value)
    #else
        #define PUB_DNSSEC_RESULT                           "%s"
        #define DNSSEC_RESULT_PARAM(dnssec_result_value)    (dnssec_result_to_description(dnssec_result_value))
    #endif
#else
        #define PUB_DNSSEC_RESULT                           "%s"
        #define DNSSEC_RESULT_PARAM(dnssec_result_value)    ("<DNSSEC Unsupported>")
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
    #if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
        #define PUB_DNSSEC_INVAL_STATE                  "%{public, mdns:dnssec_inval_state}d"
        #define DNSSEC_INVAL_STATE_PARAM(state_value)   (state_value)
    #else
        #define PUB_DNSSEC_INVAL_STATE                  "%s"
        #define DNSSEC_INVAL_STATE_PARAM(state_value)   (dnssec_insecure_validation_state_to_description(state_value))
    #endif
#else
        #define PUB_DNSSEC_INVAL_STATE                  "%s"
        #define DNSSEC_INVAL_STATE_PARAM(state_value)   ("<DNSSEC Unsupported>")
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_TIME_DUR    "%{mdns:time_duration}u"
#else
    #define PUB_TIME_DUR    "%us"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_OS_ERR    "%{mdns:err}ld"
#else
    #define PUB_OS_ERR    "%ld"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_RDATA       "%{public, mdns:rdata}.*P"
    #define PRI_RDATA       "%{sensitive, mask.hash, mdns:rdata}.*P"
    #define RDATA_PARAM(buf, buf_len, rrtype, rdata, rdata_len) \
                            (rdata_len + 2), GetPrintableRDataBytes(buf, buf_len, rrtype, rdata, rdata_len)

    #define PUB_TYPE_RDATA  "%{public, mdns:rrtype+rdata}.*P"
    #define PRI_TYPE_RDATA  "%{sensitive, mask.hash, mdns:rrtype+rdata}.*P"
    #define TYPE_RDATA_PARAM(buf, buf_len, rrtype, rdata, rdata_len) RDATA_PARAM(buf, buf_len, rrtype, rdata, rdata_len)
#else
    #define PUB_RDATA       "%p"
    #define PRI_RDATA       PUB_RDATA
    #define RDATA_PARAM(buf, buf_len, rrtype, rdata, rdata_len) (rdata)

    #define PUB_TYPE_RDATA  PUB_S " %p"
    #define PRI_TYPE_RDATA  PUB_TYPE_RDATA
    #define TYPE_RDATA_PARAM(buf, buf_len, rrtype, rdata, rdata_len) \
                            DNSTypeName(rrtype), RDATA_PARAM(buf, buf_len, rrtype, rdata, rdata_len)
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_D2D_SRV_EVENT   "%{public, mdnsresponder:d2d_service_event}d"
#endif

#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
    #define PUB_DNS_SCOPE_TYPE          "%{public, mdnsresponder:dns_scope_type}d"
    #define DNS_SCOPE_TYPE_PARAM(type)  (type)
#else
    #define PUB_DNS_SCOPE_TYPE          "%s"
    #define DNS_SCOPE_TYPE_PARAM(type)  DNSScopeToString(type)
#endif

extern void LogToFD(int fd, const char *format, ...);

#endif // __mDNSDebug_h
