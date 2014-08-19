//===-- asan_intercepted_functions.h ----------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// ASan-private header containing prototypes for wrapper functions and wrappers
//===----------------------------------------------------------------------===//
#ifndef ASAN_INTERCEPTED_FUNCTIONS_H
#define ASAN_INTERCEPTED_FUNCTIONS_H

#include "asan_internal.h"
#include "interception/interception.h"
#include "sanitizer_common/sanitizer_platform_interceptors.h"

#include <stdarg.h>
#include <stddef.h>

using __sanitizer::uptr;

// Use macro to describe if specific function should be
// intercepted on a given platform.
#if !defined(_WIN32)
# define ASAN_INTERCEPT_ATOLL_AND_STRTOLL 1
# define ASAN_INTERCEPT__LONGJMP 1
# define ASAN_INTERCEPT_STRDUP 1
# define ASAN_INTERCEPT_STRCASECMP_AND_STRNCASECMP 1
# define ASAN_INTERCEPT_INDEX 1
# define ASAN_INTERCEPT_PTHREAD_CREATE 1
# define ASAN_INTERCEPT_MLOCKX 1
#else
# define ASAN_INTERCEPT_ATOLL_AND_STRTOLL 0
# define ASAN_INTERCEPT__LONGJMP 0
# define ASAN_INTERCEPT_STRDUP 0
# define ASAN_INTERCEPT_STRCASECMP_AND_STRNCASECMP 0
# define ASAN_INTERCEPT_INDEX 0
# define ASAN_INTERCEPT_PTHREAD_CREATE 0
# define ASAN_INTERCEPT_MLOCKX 0
#endif

#if defined(__linux__)
# define ASAN_USE_ALIAS_ATTRIBUTE_FOR_INDEX 1
#else
# define ASAN_USE_ALIAS_ATTRIBUTE_FOR_INDEX 0
#endif

#if !defined(__APPLE__)
# define ASAN_INTERCEPT_STRNLEN 1
#else
# define ASAN_INTERCEPT_STRNLEN 0
#endif

#if defined(__linux__) && !defined(ANDROID)
# define ASAN_INTERCEPT_SWAPCONTEXT 1
#else
# define ASAN_INTERCEPT_SWAPCONTEXT 0
#endif

#if !defined(ANDROID) && !defined(_WIN32)
# define ASAN_INTERCEPT_SIGNAL_AND_SIGACTION 1
#else
# define ASAN_INTERCEPT_SIGNAL_AND_SIGACTION 0
#endif

#if !defined(_WIN32)
# define ASAN_INTERCEPT_SIGLONGJMP 1
#else
# define ASAN_INTERCEPT_SIGLONGJMP 0
#endif

#if ASAN_HAS_EXCEPTIONS && !defined(_WIN32)
# define ASAN_INTERCEPT___CXA_THROW 1
#else
# define ASAN_INTERCEPT___CXA_THROW 0
#endif

#define INTERPOSE_FUNCTION(function) \
    { reinterpret_cast<const uptr>(WRAP(function)), \
      reinterpret_cast<const uptr>(function) }

#define INTERPOSE_FUNCTION_2(function, wrapper) \
    { reinterpret_cast<const uptr>(wrapper), \
      reinterpret_cast<const uptr>(function) }

struct interpose_substitution {
  const uptr replacement;
  const uptr original;
};

#define INTERPOSER(func) __attribute__((used)) \
const interpose_substitution substitution_##func[] \
    __attribute__((section("__DATA, __interpose"))) = { \
  INTERPOSE_FUNCTION(func), \
}

#define INTERPOSER_2(func, wrapper) __attribute__((used)) \
const interpose_substitution substitution_##func[] \
    __attribute__((section("__DATA, __interpose"))) = { \
  INTERPOSE_FUNCTION_2(func, wrapper), \
}


#define DECLARE_FUNCTION_AND_WRAPPER(ret_type, func, ...) \
  ret_type func(__VA_ARGS__); \
  ret_type WRAP(func)(__VA_ARGS__); \
  INTERPOSER(func)

// Use extern declarations of intercepted functions on Mac and Windows
// to avoid including system headers.
#if defined(__APPLE__) || (defined(_WIN32) && !defined(_DLL))
extern "C" {
// signal.h
# if ASAN_INTERCEPT_SIGNAL_AND_SIGACTION
struct sigaction;
DECLARE_FUNCTION_AND_WRAPPER(int, sigaction, int sig,
              const struct sigaction *act,
              struct sigaction *oldact);
DECLARE_FUNCTION_AND_WRAPPER(void*, signal, int signum, void *handler);
# endif

// setjmp.h
DECLARE_FUNCTION_AND_WRAPPER(void, longjmp, void *env, int value);
# if ASAN_INTERCEPT__LONGJMP
DECLARE_FUNCTION_AND_WRAPPER(void, _longjmp, void *env, int value);
# endif
# if ASAN_INTERCEPT_SIGLONGJMP
DECLARE_FUNCTION_AND_WRAPPER(void, siglongjmp, void *env, int value);
# endif
# if ASAN_INTERCEPT___CXA_THROW
DECLARE_FUNCTION_AND_WRAPPER(void, __cxa_throw, void *a, void *b, void *c);
# endif

// string.h / strings.h
DECLARE_FUNCTION_AND_WRAPPER(int, memcmp,
                             const void *a1, const void *a2, uptr size);
DECLARE_FUNCTION_AND_WRAPPER(void*, memmove,
                             void *to, const void *from, uptr size);
DECLARE_FUNCTION_AND_WRAPPER(void*, memcpy,
                             void *to, const void *from, uptr size);
DECLARE_FUNCTION_AND_WRAPPER(void*, memset, void *block, int c, uptr size);
DECLARE_FUNCTION_AND_WRAPPER(char*, strchr, const char *str, int c);
DECLARE_FUNCTION_AND_WRAPPER(char*, strcat,  /* NOLINT */
                             char *to, const char* from);
DECLARE_FUNCTION_AND_WRAPPER(char*, strncat,
                             char *to, const char* from, uptr size);
DECLARE_FUNCTION_AND_WRAPPER(char*, strcpy,  /* NOLINT */
                             char *to, const char* from);
DECLARE_FUNCTION_AND_WRAPPER(char*, strncpy,
                             char *to, const char* from, uptr size);
DECLARE_FUNCTION_AND_WRAPPER(int, strcmp, const char *s1, const char* s2);
DECLARE_FUNCTION_AND_WRAPPER(int, strncmp,
                             const char *s1, const char* s2, uptr size);
DECLARE_FUNCTION_AND_WRAPPER(uptr, strlen, const char *s);
# if ASAN_INTERCEPT_STRCASECMP_AND_STRNCASECMP
DECLARE_FUNCTION_AND_WRAPPER(int, strcasecmp, const char *s1, const char *s2);
DECLARE_FUNCTION_AND_WRAPPER(int, strncasecmp,
                             const char *s1, const char *s2, uptr n);
# endif
# if ASAN_INTERCEPT_STRDUP
DECLARE_FUNCTION_AND_WRAPPER(char*, strdup, const char *s);
# endif
# if ASAN_INTERCEPT_STRNLEN
DECLARE_FUNCTION_AND_WRAPPER(uptr, strnlen, const char *s, uptr maxlen);
# endif
# if ASAN_INTERCEPT_INDEX
char* index(const char *string, int c);
INTERPOSER_2(index, WRAP(strchr));
# endif

// stdlib.h
DECLARE_FUNCTION_AND_WRAPPER(int, atoi, const char *nptr);
DECLARE_FUNCTION_AND_WRAPPER(long, atol, const char *nptr);  // NOLINT
DECLARE_FUNCTION_AND_WRAPPER(long, strtol, const char *nptr, char **endptr, int base);  // NOLINT
# if ASAN_INTERCEPT_ATOLL_AND_STRTOLL
DECLARE_FUNCTION_AND_WRAPPER(long long, atoll, const char *nptr);  // NOLINT
DECLARE_FUNCTION_AND_WRAPPER(long long, strtoll, const char *nptr, char **endptr, int base);  // NOLINT
# endif

// unistd.h
# if SANITIZER_INTERCEPT_READ
DECLARE_FUNCTION_AND_WRAPPER(SSIZE_T, read, int fd, void *buf, SIZE_T count);
# endif
# if SANITIZER_INTERCEPT_PREAD
DECLARE_FUNCTION_AND_WRAPPER(SSIZE_T, pread, int fd, void *buf,
                             SIZE_T count, OFF_T offset);
# endif
# if SANITIZER_INTERCEPT_PREAD64
DECLARE_FUNCTION_AND_WRAPPER(SSIZE_T, pread64, int fd, void *buf,
                             SIZE_T count, OFF64_T offset);
# endif

# if SANITIZER_INTERCEPT_WRITE
DECLARE_FUNCTION_AND_WRAPPER(SSIZE_T, write, int fd, void *ptr, SIZE_T count);
# endif
# if SANITIZER_INTERCEPT_PWRITE
DECLARE_FUNCTION_AND_WRAPPER(SSIZE_T, pwrite,
                             int fd, void *ptr, SIZE_T count, OFF_T offset);
# endif

# if ASAN_INTERCEPT_MLOCKX
// mlock/munlock
DECLARE_FUNCTION_AND_WRAPPER(int, mlock, const void *addr, SIZE_T len);
DECLARE_FUNCTION_AND_WRAPPER(int, munlock, const void *addr, SIZE_T len);
DECLARE_FUNCTION_AND_WRAPPER(int, mlockall, int flags);
DECLARE_FUNCTION_AND_WRAPPER(int, munlockall, void);
# endif

// Windows threads.
# if defined(_WIN32)
__declspec(dllimport)
void* __stdcall CreateThread(void *sec, uptr st, void* start,
                             void *arg, DWORD fl, DWORD *id);
# endif
// Posix threads.
# if ASAN_INTERCEPT_PTHREAD_CREATE
DECLARE_FUNCTION_AND_WRAPPER(int, pthread_create,
                             void *thread, void *attr,
                             void *(*start_routine)(void*), void *arg);
# endif

# if SANITIZER_INTERCEPT_LOCALTIME_AND_FRIENDS
DECLARE_FUNCTION_AND_WRAPPER(void *, localtime, unsigned long *timep);
DECLARE_FUNCTION_AND_WRAPPER(void *, localtime_r, unsigned long *timep,
                             void *result);
DECLARE_FUNCTION_AND_WRAPPER(void *, gmtime, unsigned long *timep);
DECLARE_FUNCTION_AND_WRAPPER(void *, gmtime_r, unsigned long *timep,
                             void *result);
DECLARE_FUNCTION_AND_WRAPPER(char *, ctime, unsigned long *timep);
DECLARE_FUNCTION_AND_WRAPPER(char *, ctime_r, unsigned long *timep,
                             char *result);
DECLARE_FUNCTION_AND_WRAPPER(char *, asctime, void *tm);
DECLARE_FUNCTION_AND_WRAPPER(char *, asctime_r, void *tm, char *result);
# endif

// stdio.h
# if SANITIZER_INTERCEPT_SCANF
DECLARE_FUNCTION_AND_WRAPPER(int, vscanf, const char *format, va_list ap);
DECLARE_FUNCTION_AND_WRAPPER(int, vsscanf, const char *str, const char *format,
                             va_list ap);
DECLARE_FUNCTION_AND_WRAPPER(int, vfscanf, void *stream, const char *format,
                             va_list ap);
DECLARE_FUNCTION_AND_WRAPPER(int, scanf, const char *format, ...);
DECLARE_FUNCTION_AND_WRAPPER(int, fscanf,
                             void* stream, const char *format, ...);
DECLARE_FUNCTION_AND_WRAPPER(int, sscanf,  // NOLINT
                             const char *str, const char *format, ...);
# endif

# if defined(__APPLE__)
typedef void* pthread_workqueue_t;
typedef void* pthread_workitem_handle_t;

typedef void* dispatch_group_t;
typedef void* dispatch_queue_t;
typedef void* dispatch_source_t;
typedef u64 dispatch_time_t;
typedef void (*dispatch_function_t)(void *block);
typedef void* (*worker_t)(void *block);

DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_async_f,
                             dispatch_queue_t dq,
                             void *ctxt, dispatch_function_t func);
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_sync_f,
                             dispatch_queue_t dq,
                             void *ctxt, dispatch_function_t func);
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_after_f,
                             dispatch_time_t when, dispatch_queue_t dq,
                             void *ctxt, dispatch_function_t func);
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_barrier_async_f,
                             dispatch_queue_t dq,
                             void *ctxt, dispatch_function_t func);
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_group_async_f,
                             dispatch_group_t group, dispatch_queue_t dq,
                             void *ctxt, dispatch_function_t func);

#  if !defined(MISSING_BLOCKS_SUPPORT)
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_group_async,
                             dispatch_group_t dg,
                             dispatch_queue_t dq, void (^work)(void));
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_async,
                             dispatch_queue_t dq, void (^work)(void));
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_after,
                             dispatch_queue_t dq, void (^work)(void));
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_source_set_event_handler,
                             dispatch_source_t ds, void (^work)(void));
DECLARE_FUNCTION_AND_WRAPPER(void, dispatch_source_set_cancel_handler,
                             dispatch_source_t ds, void (^work)(void));
#  endif  // MISSING_BLOCKS_SUPPORT

typedef void malloc_zone_t;
typedef size_t vm_size_t;
DECLARE_FUNCTION_AND_WRAPPER(malloc_zone_t *, malloc_create_zone,
                             vm_size_t start_size, unsigned flags);
DECLARE_FUNCTION_AND_WRAPPER(malloc_zone_t *, malloc_default_zone, void);
DECLARE_FUNCTION_AND_WRAPPER(
    malloc_zone_t *, malloc_default_purgeable_zone, void);
DECLARE_FUNCTION_AND_WRAPPER(void, malloc_make_purgeable, void *ptr);
DECLARE_FUNCTION_AND_WRAPPER(int, malloc_make_nonpurgeable, void *ptr);
DECLARE_FUNCTION_AND_WRAPPER(void, malloc_set_zone_name,
                             malloc_zone_t *zone, const char *name);
DECLARE_FUNCTION_AND_WRAPPER(void *, malloc, size_t size);
DECLARE_FUNCTION_AND_WRAPPER(void, free, void *ptr);
DECLARE_FUNCTION_AND_WRAPPER(void *, realloc, void *ptr, size_t size);
DECLARE_FUNCTION_AND_WRAPPER(void *, calloc, size_t nmemb, size_t size);
DECLARE_FUNCTION_AND_WRAPPER(void *, valloc, size_t size);
DECLARE_FUNCTION_AND_WRAPPER(size_t, malloc_good_size, size_t size);
DECLARE_FUNCTION_AND_WRAPPER(int, posix_memalign,
                             void **memptr, size_t alignment, size_t size);
#if 0
DECLARE_FUNCTION_AND_WRAPPER(void, _malloc_fork_prepare, void);
DECLARE_FUNCTION_AND_WRAPPER(void, _malloc_fork_parent, void);
DECLARE_FUNCTION_AND_WRAPPER(void, _malloc_fork_child, void);
#endif



# endif  // __APPLE__
}  // extern "C"
#endif  // defined(__APPLE__) || (defined(_WIN32) && !defined(_DLL))

#endif  // ASAN_INTERCEPTED_FUNCTIONS_H
