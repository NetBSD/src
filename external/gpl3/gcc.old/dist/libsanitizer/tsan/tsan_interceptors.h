#ifndef TSAN_INTERCEPTORS_H
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "tsan_rtl.h"

namespace __tsan {

class ScopedInterceptor {
 public:
  ScopedInterceptor(ThreadState *thr, const char *fname, uptr pc);
  ~ScopedInterceptor();
 private:
  ThreadState *const thr_;
  const uptr pc_;
  bool in_ignored_lib_;
};

}  // namespace __tsan

#define SCOPED_INTERCEPTOR_RAW(func, ...) \
    ThreadState *thr = cur_thread(); \
    const uptr caller_pc = GET_CALLER_PC(); \
    ScopedInterceptor si(thr, #func, caller_pc); \
    const uptr pc = StackTrace::GetCurrentPc(); \
    (void)pc; \
/**/

#define SCOPED_TSAN_INTERCEPTOR(func, ...) \
    SCOPED_INTERCEPTOR_RAW(func, __VA_ARGS__); \
    if (REAL(func) == 0) { \
      Report("FATAL: ThreadSanitizer: failed to intercept %s\n", #func); \
      Die(); \
    }                                                    \
    if (thr->ignore_interceptors || thr->in_ignored_lib) \
      return REAL(func)(__VA_ARGS__); \
/**/

#define TSAN_INTERCEPTOR(ret, func, ...) INTERCEPTOR(ret, func, __VA_ARGS__)

#if SANITIZER_FREEBSD
#define __libc_free __free
#define __libc_malloc __malloc
#endif

extern "C" void __libc_free(void *ptr);
extern "C" void *__libc_malloc(uptr size);

#endif  // TSAN_INTERCEPTORS_H
