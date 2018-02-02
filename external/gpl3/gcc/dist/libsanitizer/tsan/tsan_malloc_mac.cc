//===-- tsan_malloc_mac.cc ------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// Mac-specific malloc interception.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_MAC

#include "tsan_interceptors.h"
#include "tsan_stack_trace.h"

using namespace __tsan;
#define COMMON_MALLOC_ZONE_NAME "tsan"
#define COMMON_MALLOC_ENTER()
#define COMMON_MALLOC_SANITIZER_INITIALIZED (cur_thread()->is_inited)
#define COMMON_MALLOC_FORCE_LOCK()
#define COMMON_MALLOC_FORCE_UNLOCK()
#define COMMON_MALLOC_MEMALIGN(alignment, size) \
  void *p =                                     \
      user_alloc(cur_thread(), StackTrace::GetCurrentPc(), size, alignment)
#define COMMON_MALLOC_MALLOC(size)      \
  if (cur_thread()->in_symbolizer)      \
    return REAL(malloc)(size);          \
  SCOPED_INTERCEPTOR_RAW(malloc, size); \
  void *p = user_alloc(thr, pc, size)
#define COMMON_MALLOC_REALLOC(ptr, size)      \
  if (cur_thread()->in_symbolizer)            \
    return REAL(realloc)(ptr, size);          \
  SCOPED_INTERCEPTOR_RAW(realloc, ptr, size); \
  void *p = user_realloc(thr, pc, ptr, size)
#define COMMON_MALLOC_CALLOC(count, size)      \
  if (cur_thread()->in_symbolizer)             \
    return REAL(calloc)(count, size);          \
  SCOPED_INTERCEPTOR_RAW(calloc, size, count); \
  void *p = user_calloc(thr, pc, size, count)
#define COMMON_MALLOC_VALLOC(size)                          \
  if (cur_thread()->in_symbolizer)                          \
    return REAL(valloc)(size);                              \
  SCOPED_INTERCEPTOR_RAW(valloc, size);                     \
  void *p = user_alloc(thr, pc, size, GetPageSizeCached())
#define COMMON_MALLOC_FREE(ptr)      \
  if (cur_thread()->in_symbolizer)   \
    return REAL(free)(ptr);          \
  SCOPED_INTERCEPTOR_RAW(free, ptr); \
  user_free(thr, pc, ptr)
#define COMMON_MALLOC_SIZE(ptr) \
  uptr size = user_alloc_usable_size(ptr);
#define COMMON_MALLOC_FILL_STATS(zone, stats)
#define COMMON_MALLOC_REPORT_UNKNOWN_REALLOC(ptr, zone_ptr, zone_name) \
  (void)zone_name; \
  Report("mz_realloc(%p) -- attempting to realloc unallocated memory.\n", ptr);
#define COMMON_MALLOC_IGNORE_INVALID_FREE false
#define COMMON_MALLOC_REPORT_FREE_UNALLOCATED(ptr, zone_ptr, zone_name) \
  (void)zone_name; \
  Report("free_common(%p) -- attempting to free unallocated memory.\n", ptr);
#define COMMON_MALLOC_NAMESPACE __tsan

#include "sanitizer_common/sanitizer_malloc_mac.inc"

#endif
