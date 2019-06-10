//===-- tsan_interceptors.cc ----------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// FIXME: move as many interceptors as possible into
// sanitizer_common/sanitizer_common_interceptors.inc
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_linux.h"
#include "sanitizer_common/sanitizer_platform_limits_posix.h"
#include "sanitizer_common/sanitizer_placement_new.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "interception/interception.h"
#include "tsan_interceptors.h"
#include "tsan_interface.h"
#include "tsan_platform.h"
#include "tsan_suppressions.h"
#include "tsan_rtl.h"
#include "tsan_mman.h"
#include "tsan_fd.h"

#if SANITIZER_POSIX
#include "sanitizer_common/sanitizer_posix.h"
#endif

using namespace __tsan;  // NOLINT

#if SANITIZER_FREEBSD || SANITIZER_MAC
#define __errno_location __error
#define stdout __stdoutp
#define stderr __stderrp
#endif

#if SANITIZER_FREEBSD
#define __libc_realloc __realloc
#define __libc_calloc __calloc
#elif SANITIZER_MAC
#define __libc_malloc REAL(malloc)
#define __libc_realloc REAL(realloc)
#define __libc_calloc REAL(calloc)
#define __libc_free REAL(free)
#endif

#if SANITIZER_LINUX || SANITIZER_FREEBSD
#define PTHREAD_CREATE_DETACHED 1
#elif SANITIZER_MAC
#define PTHREAD_CREATE_DETACHED 2
#endif
#if SANITIZER_NETBSD
#define	__errno_location __errno
#define	pthread_yield sched_yield
#define	fileno_unlocked fileno
#define	stdout __sF[1]
#define	stderr __sF[2]
#endif


#ifdef __mips__
const int kSigCount = 129;
#else
const int kSigCount = 65;
#endif

struct my_siginfo_t {
  // The size is determined by looking at sizeof of real siginfo_t on linux.
  u64 opaque[128 / sizeof(u64)];
};

#ifdef __mips__
struct ucontext_t {
  u64 opaque[768 / sizeof(u64) + 1];
};
#else
struct ucontext_t {
  // The size is determined by looking at sizeof of real ucontext_t on linux.
  u64 opaque[936 / sizeof(u64) + 1];
};
#endif

#if defined(__x86_64__) || defined(__mips__)
#define PTHREAD_ABI_BASE  "GLIBC_2.3.2"
#elif defined(__aarch64__)
#define PTHREAD_ABI_BASE  "GLIBC_2.17"
#endif

extern "C" int pthread_attr_init(void *attr);
extern "C" int pthread_attr_destroy(void *attr);
DECLARE_REAL(int, pthread_attr_getdetachstate, void *, void *)
extern "C" int pthread_attr_setstacksize(void *attr, uptr stacksize);
extern "C" int pthread_key_create(unsigned *key, void (*destructor)(void* v));
extern "C" int pthread_setspecific(unsigned key, const void *v);
DECLARE_REAL(int, pthread_mutexattr_gettype, void *, void *)
extern "C" int pthread_sigmask(int how, const __sanitizer_sigset_t *set,
                               __sanitizer_sigset_t *oldset);
// REAL(sigfillset) defined in common interceptors.
DECLARE_REAL(int, sigfillset, __sanitizer_sigset_t *set)
DECLARE_REAL(int, fflush, __sanitizer_FILE *fp)
DECLARE_REAL_AND_INTERCEPTOR(void *, malloc, uptr size)
DECLARE_REAL_AND_INTERCEPTOR(void, free, void *ptr)
extern "C" void *pthread_self();
extern "C" void _exit(int status);
extern "C" int *__errno_location();
extern "C" int fileno_unlocked(void *stream);
extern "C" void *__libc_calloc(uptr size, uptr n);
extern "C" void *__libc_realloc(void *ptr, uptr size);
extern "C" void __libc_free(void *ptr);
extern "C" int dirfd(void *dirp);
#if !SANITIZER_FREEBSD && !SANITIZER_NETBSD
extern "C" int mallopt(int param, int value);
#endif
#if SANITIZER_NETBSD
extern __sanitizer_FILE **__sF;
#else
extern __sanitizer_FILE *stdout, *stderr;
#endif
const int PTHREAD_MUTEX_RECURSIVE = 1;
const int PTHREAD_MUTEX_RECURSIVE_NP = 1;
const int EINVAL = 22;
const int EBUSY = 16;
const int EOWNERDEAD = 130;
#if !SANITIZER_MAC
const int EPOLL_CTL_ADD = 1;
#endif
const int SIGILL = 4;
const int SIGABRT = 6;
const int SIGFPE = 8;
const int SIGSEGV = 11;
const int SIGPIPE = 13;
const int SIGTERM = 15;
#ifdef __mips__
const int SIGBUS = 10;
const int SIGSYS = 12;
#else
const int SIGBUS = 7;
const int SIGSYS = 31;
#endif
void *const MAP_FAILED = (void*)-1;
#if !SANITIZER_MAC
const int PTHREAD_BARRIER_SERIAL_THREAD = -1;
#endif
const int MAP_FIXED = 0x10;
typedef long long_t;  // NOLINT

// From /usr/include/unistd.h
# define F_ULOCK 0      /* Unlock a previously locked region.  */
# define F_LOCK  1      /* Lock a region for exclusive use.  */
# define F_TLOCK 2      /* Test and lock a region for exclusive use.  */
# define F_TEST  3      /* Test a region for other processes locks.  */

#define errno (*__errno_location())

typedef void (*sighandler_t)(int sig);
typedef void (*sigactionhandler_t)(int sig, my_siginfo_t *siginfo, void *uctx);

struct sigaction_t {
#ifdef __mips__
  u32 sa_flags;
#endif
  union {
    sighandler_t sa_handler;
    sigactionhandler_t sa_sigaction;
  };
#if SANITIZER_FREEBSD || SANITIZER_NETBSD
  int sa_flags;
  __sanitizer_sigset_t sa_mask;
#else
  __sanitizer_sigset_t sa_mask;
#ifndef __mips__
  int sa_flags;
#endif
  void (*sa_restorer)();
#endif
};

const sighandler_t SIG_DFL = (sighandler_t)0;
const sighandler_t SIG_IGN = (sighandler_t)1;
const sighandler_t SIG_ERR = (sighandler_t)-1;
#if SANITIZER_FREEBSD
const int SA_SIGINFO = 0x40;
const int SIG_SETMASK = 3;
#elif defined(__mips__)
const int SA_SIGINFO = 8;
const int SIG_SETMASK = 3;
#else
const int SA_SIGINFO = 4;
const int SIG_SETMASK = 2;
#endif

#define COMMON_INTERCEPTOR_NOTHING_IS_INITIALIZED \
  (!cur_thread()->is_inited)

static sigaction_t sigactions[kSigCount];

namespace __tsan {
struct SignalDesc {
  bool armed;
  bool sigaction;
  my_siginfo_t siginfo;
  ucontext_t ctx;
};

struct ThreadSignalContext {
  int int_signal_send;
  atomic_uintptr_t in_blocking_func;
  atomic_uintptr_t have_pending_signals;
  SignalDesc pending_signals[kSigCount];
};

// The object is 64-byte aligned, because we want hot data to be located in
// a single cache line if possible (it's accessed in every interceptor).
static ALIGNED(64) char libignore_placeholder[sizeof(LibIgnore)];
static LibIgnore *libignore() {
  return reinterpret_cast<LibIgnore*>(&libignore_placeholder[0]);
}

void InitializeLibIgnore() {
  const SuppressionContext &supp = *Suppressions();
  const uptr n = supp.SuppressionCount();
  for (uptr i = 0; i < n; i++) {
    const Suppression *s = supp.SuppressionAt(i);
    if (0 == internal_strcmp(s->type, kSuppressionLib))
      libignore()->AddIgnoredLibrary(s->templ);
  }
  libignore()->OnLibraryLoaded(0);
}

}  // namespace __tsan

static ThreadSignalContext *SigCtx(ThreadState *thr) {
  ThreadSignalContext *ctx = (ThreadSignalContext*)thr->signal_ctx;
  if (ctx == 0 && !thr->is_dead) {
    ctx = (ThreadSignalContext*)MmapOrDie(sizeof(*ctx), "ThreadSignalContext");
    MemoryResetRange(thr, (uptr)&SigCtx, (uptr)ctx, sizeof(*ctx));
    thr->signal_ctx = ctx;
  }
  return ctx;
}

static unsigned g_thread_finalize_key;

ScopedInterceptor::ScopedInterceptor(ThreadState *thr, const char *fname,
                                     uptr pc)
    : thr_(thr)
    , pc_(pc)
    , in_ignored_lib_(false) {
  if (!thr_->ignore_interceptors) {
    Initialize(thr);
    FuncEntry(thr, pc);
  }
  DPrintf("#%d: intercept %s()\n", thr_->tid, fname);
  if (!thr_->in_ignored_lib && libignore()->IsIgnored(pc)) {
    in_ignored_lib_ = true;
    thr_->in_ignored_lib = true;
    ThreadIgnoreBegin(thr_, pc_);
  }
}

ScopedInterceptor::~ScopedInterceptor() {
  if (in_ignored_lib_) {
    thr_->in_ignored_lib = false;
    ThreadIgnoreEnd(thr_, pc_);
  }
  if (!thr_->ignore_interceptors) {
    ProcessPendingSignals(thr_);
    FuncExit(thr_);
    CheckNoLocks(thr_);
  }
}

#define TSAN_INTERCEPT(func) INTERCEPT_FUNCTION(func)
#if SANITIZER_FREEBSD || SANITIZER_NETBSD
# define TSAN_INTERCEPT_VER(func, ver) INTERCEPT_FUNCTION(func)
#else
# define TSAN_INTERCEPT_VER(func, ver) INTERCEPT_FUNCTION_VER(func, ver)
#endif

#define READ_STRING_OF_LEN(thr, pc, s, len, n)                 \
  MemoryAccessRange((thr), (pc), (uptr)(s),                         \
    common_flags()->strict_string_checks ? (len) + 1 : (n), false)

#define READ_STRING(thr, pc, s, n)                             \
    READ_STRING_OF_LEN((thr), (pc), (s), internal_strlen(s), (n))

#define BLOCK_REAL(name) (BlockingCall(thr), REAL(name))

struct BlockingCall {
  explicit BlockingCall(ThreadState *thr)
      : thr(thr)
      , ctx(SigCtx(thr)) {
    for (;;) {
      atomic_store(&ctx->in_blocking_func, 1, memory_order_relaxed);
      if (atomic_load(&ctx->have_pending_signals, memory_order_relaxed) == 0)
        break;
      atomic_store(&ctx->in_blocking_func, 0, memory_order_relaxed);
      ProcessPendingSignals(thr);
    }
    // When we are in a "blocking call", we process signals asynchronously
    // (right when they arrive). In this context we do not expect to be
    // executing any user/runtime code. The known interceptor sequence when
    // this is not true is: pthread_join -> munmap(stack). It's fine
    // to ignore munmap in this case -- we handle stack shadow separately.
    thr->ignore_interceptors++;
  }

  ~BlockingCall() {
    thr->ignore_interceptors--;
    atomic_store(&ctx->in_blocking_func, 0, memory_order_relaxed);
  }

  ThreadState *thr;
  ThreadSignalContext *ctx;
};

TSAN_INTERCEPTOR(unsigned, sleep, unsigned sec) {
  SCOPED_TSAN_INTERCEPTOR(sleep, sec);
  unsigned res = BLOCK_REAL(sleep)(sec);
  AfterSleep(thr, pc);
  return res;
}

TSAN_INTERCEPTOR(int, usleep, long_t usec) {
  SCOPED_TSAN_INTERCEPTOR(usleep, usec);
  int res = BLOCK_REAL(usleep)(usec);
  AfterSleep(thr, pc);
  return res;
}

TSAN_INTERCEPTOR(int, nanosleep, void *req, void *rem) {
  SCOPED_TSAN_INTERCEPTOR(nanosleep, req, rem);
  int res = BLOCK_REAL(nanosleep)(req, rem);
  AfterSleep(thr, pc);
  return res;
}

// The sole reason tsan wraps atexit callbacks is to establish synchronization
// between callback setup and callback execution.
struct AtExitCtx {
  void (*f)();
  void *arg;
};

static void at_exit_wrapper(void *arg) {
  ThreadState *thr = cur_thread();
  uptr pc = 0;
  Acquire(thr, pc, (uptr)arg);
  AtExitCtx *ctx = (AtExitCtx*)arg;
  ((void(*)(void *arg))ctx->f)(ctx->arg);
  __libc_free(ctx);
}

static int setup_at_exit_wrapper(ThreadState *thr, uptr pc, void(*f)(),
      void *arg, void *dso);

TSAN_INTERCEPTOR(int, atexit, void (*f)()) {
  if (cur_thread()->in_symbolizer)
    return 0;
  // We want to setup the atexit callback even if we are in ignored lib
  // or after fork.
  SCOPED_INTERCEPTOR_RAW(atexit, f);
  return setup_at_exit_wrapper(thr, pc, (void(*)())f, 0, 0);
}

TSAN_INTERCEPTOR(int, __cxa_atexit, void (*f)(void *a), void *arg, void *dso) {
  if (cur_thread()->in_symbolizer)
    return 0;
  SCOPED_TSAN_INTERCEPTOR(__cxa_atexit, f, arg, dso);
  return setup_at_exit_wrapper(thr, pc, (void(*)())f, arg, dso);
}

static int setup_at_exit_wrapper(ThreadState *thr, uptr pc, void(*f)(),
      void *arg, void *dso) {
  AtExitCtx *ctx = (AtExitCtx*)__libc_malloc(sizeof(AtExitCtx));
  ctx->f = f;
  ctx->arg = arg;
  Release(thr, pc, (uptr)ctx);
  // Memory allocation in __cxa_atexit will race with free during exit,
  // because we do not see synchronization around atexit callback list.
  ThreadIgnoreBegin(thr, pc);
  int res = REAL(__cxa_atexit)(at_exit_wrapper, ctx, dso);
  ThreadIgnoreEnd(thr, pc);
  return res;
}

#if !SANITIZER_MAC
static void on_exit_wrapper(int status, void *arg) {
  ThreadState *thr = cur_thread();
  uptr pc = 0;
  Acquire(thr, pc, (uptr)arg);
  AtExitCtx *ctx = (AtExitCtx*)arg;
  ((void(*)(int status, void *arg))ctx->f)(status, ctx->arg);
  __libc_free(ctx);
}

TSAN_INTERCEPTOR(int, on_exit, void(*f)(int, void*), void *arg) {
  if (cur_thread()->in_symbolizer)
    return 0;
  SCOPED_TSAN_INTERCEPTOR(on_exit, f, arg);
  AtExitCtx *ctx = (AtExitCtx*)__libc_malloc(sizeof(AtExitCtx));
  ctx->f = (void(*)())f;
  ctx->arg = arg;
  Release(thr, pc, (uptr)ctx);
  // Memory allocation in __cxa_atexit will race with free during exit,
  // because we do not see synchronization around atexit callback list.
  ThreadIgnoreBegin(thr, pc);
  int res = REAL(on_exit)(on_exit_wrapper, ctx);
  ThreadIgnoreEnd(thr, pc);
  return res;
}
#endif

// Cleanup old bufs.
static void JmpBufGarbageCollect(ThreadState *thr, uptr sp) {
  for (uptr i = 0; i < thr->jmp_bufs.Size(); i++) {
    JmpBuf *buf = &thr->jmp_bufs[i];
    if (buf->sp <= sp) {
      uptr sz = thr->jmp_bufs.Size();
      thr->jmp_bufs[i] = thr->jmp_bufs[sz - 1];
      thr->jmp_bufs.PopBack();
      i--;
    }
  }
}

static void SetJmp(ThreadState *thr, uptr sp, uptr mangled_sp) {
  if (!thr->is_inited)  // called from libc guts during bootstrap
    return;
  // Cleanup old bufs.
  JmpBufGarbageCollect(thr, sp);
  // Remember the buf.
  JmpBuf *buf = thr->jmp_bufs.PushBack();
  buf->sp = sp;
  buf->mangled_sp = mangled_sp;
  buf->shadow_stack_pos = thr->shadow_stack_pos;
  ThreadSignalContext *sctx = SigCtx(thr);
  buf->int_signal_send = sctx ? sctx->int_signal_send : 0;
  buf->in_blocking_func = sctx ?
      atomic_load(&sctx->in_blocking_func, memory_order_relaxed) :
      false;
  buf->in_signal_handler = atomic_load(&thr->in_signal_handler,
      memory_order_relaxed);
}

static void LongJmp(ThreadState *thr, uptr *env) {
#if SANITIZER_FREEBSD || SANITIZER_NETBSD
  uptr mangled_sp = env[2];
#elif defined(SANITIZER_LINUX)
# ifdef __aarch64__
  uptr mangled_sp = env[13];
# else
  uptr mangled_sp = env[6];
# endif
#endif  // SANITIZER_FREEBSD
  // Find the saved buf by mangled_sp.
  for (uptr i = 0; i < thr->jmp_bufs.Size(); i++) {
    JmpBuf *buf = &thr->jmp_bufs[i];
    if (buf->mangled_sp == mangled_sp) {
      CHECK_GE(thr->shadow_stack_pos, buf->shadow_stack_pos);
      // Unwind the stack.
      while (thr->shadow_stack_pos > buf->shadow_stack_pos)
        FuncExit(thr);
      ThreadSignalContext *sctx = SigCtx(thr);
      if (sctx) {
        sctx->int_signal_send = buf->int_signal_send;
        atomic_store(&sctx->in_blocking_func, buf->in_blocking_func,
            memory_order_relaxed);
      }
      atomic_store(&thr->in_signal_handler, buf->in_signal_handler,
          memory_order_relaxed);
      JmpBufGarbageCollect(thr, buf->sp - 1);  // do not collect buf->sp
      return;
    }
  }
  Printf("ThreadSanitizer: can't find longjmp buf\n");
  CHECK(0);
}

// FIXME: put everything below into a common extern "C" block?
extern "C" void __tsan_setjmp(uptr sp, uptr mangled_sp) {
  SetJmp(cur_thread(), sp, mangled_sp);
}

// Not called.  Merely to satisfy TSAN_INTERCEPT().
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
int __interceptor_setjmp(void *env);
extern "C" int __interceptor_setjmp(void *env) {
  CHECK(0);
  return 0;
}

// FIXME: any reason to have a separate declaration?
extern "C" SANITIZER_INTERFACE_ATTRIBUTE
int __interceptor__setjmp(void *env);
extern "C" int __interceptor__setjmp(void *env) {
  CHECK(0);
  return 0;
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
int __interceptor_sigsetjmp(void *env);
extern "C" int __interceptor_sigsetjmp(void *env) {
  CHECK(0);
  return 0;
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
int __interceptor___sigsetjmp(void *env);
extern "C" int __interceptor___sigsetjmp(void *env) {
  CHECK(0);
  return 0;
}

extern "C" int setjmp(void *env);
extern "C" int _setjmp(void *env);
extern "C" int sigsetjmp(void *env);
extern "C" int __sigsetjmp(void *env);
DEFINE_REAL(int, setjmp, void *env)
DEFINE_REAL(int, _setjmp, void *env)
DEFINE_REAL(int, sigsetjmp, void *env)
DEFINE_REAL(int, __sigsetjmp, void *env)

TSAN_INTERCEPTOR(void, longjmp, uptr *env, int val) {
  {
    SCOPED_TSAN_INTERCEPTOR(longjmp, env, val);
  }
  LongJmp(cur_thread(), env);
  REAL(longjmp)(env, val);
}

TSAN_INTERCEPTOR(void, siglongjmp, uptr *env, int val) {
  {
    SCOPED_TSAN_INTERCEPTOR(siglongjmp, env, val);
  }
  LongJmp(cur_thread(), env);
  REAL(siglongjmp)(env, val);
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(void*, malloc, uptr size) {
  if (cur_thread()->in_symbolizer)
    return __libc_malloc(size);
  void *p = 0;
  {
    SCOPED_INTERCEPTOR_RAW(malloc, size);
    p = user_alloc(thr, pc, size);
  }
  invoke_malloc_hook(p, size);
  return p;
}

TSAN_INTERCEPTOR(void*, __libc_memalign, uptr align, uptr sz) {
  SCOPED_TSAN_INTERCEPTOR(__libc_memalign, align, sz);
  return user_alloc(thr, pc, sz, align);
}

TSAN_INTERCEPTOR(void*, calloc, uptr size, uptr n) {
  if (cur_thread()->in_symbolizer)
    return __libc_calloc(size, n);
  void *p = 0;
  {
    SCOPED_INTERCEPTOR_RAW(calloc, size, n);
    p = user_calloc(thr, pc, size, n);
  }
  invoke_malloc_hook(p, n * size);
  return p;
}

TSAN_INTERCEPTOR(void*, realloc, void *p, uptr size) {
  if (cur_thread()->in_symbolizer)
    return __libc_realloc(p, size);
  if (p)
    invoke_free_hook(p);
  {
    SCOPED_INTERCEPTOR_RAW(realloc, p, size);
    p = user_realloc(thr, pc, p, size);
  }
  invoke_malloc_hook(p, size);
  return p;
}

TSAN_INTERCEPTOR(void, free, void *p) {
  if (p == 0)
    return;
  if (cur_thread()->in_symbolizer)
    return __libc_free(p);
  invoke_free_hook(p);
  SCOPED_INTERCEPTOR_RAW(free, p);
  user_free(thr, pc, p);
}

TSAN_INTERCEPTOR(void, cfree, void *p) {
  if (p == 0)
    return;
  if (cur_thread()->in_symbolizer)
    return __libc_free(p);
  invoke_free_hook(p);
  SCOPED_INTERCEPTOR_RAW(cfree, p);
  user_free(thr, pc, p);
}

TSAN_INTERCEPTOR(uptr, malloc_usable_size, void *p) {
  SCOPED_INTERCEPTOR_RAW(malloc_usable_size, p);
  return user_alloc_usable_size(p);
}
#endif

TSAN_INTERCEPTOR(uptr, strlen, const char *s) {
  SCOPED_TSAN_INTERCEPTOR(strlen, s);
  uptr len = internal_strlen(s);
  MemoryAccessRange(thr, pc, (uptr)s, len + 1, false);
  return len;
}

TSAN_INTERCEPTOR(void*, memset, void *dst, int v, uptr size) {
  // On FreeBSD we get here from libthr internals on thread initialization.
  if (!COMMON_INTERCEPTOR_NOTHING_IS_INITIALIZED) {
    SCOPED_TSAN_INTERCEPTOR(memset, dst, v, size);
    MemoryAccessRange(thr, pc, (uptr)dst, size, true);
  }
  return internal_memset(dst, v, size);
}

TSAN_INTERCEPTOR(void*, memcpy, void *dst, const void *src, uptr size) {
  // On FreeBSD we get here from libthr internals on thread initialization.
  if (!COMMON_INTERCEPTOR_NOTHING_IS_INITIALIZED) {
    SCOPED_TSAN_INTERCEPTOR(memcpy, dst, src, size);
    MemoryAccessRange(thr, pc, (uptr)dst, size, true);
    MemoryAccessRange(thr, pc, (uptr)src, size, false);
  }
  // On OS X, calling internal_memcpy here will cause memory corruptions,
  // because memcpy and memmove are actually aliases of the same implementation.
  // We need to use internal_memmove here.
  return internal_memmove(dst, src, size);
}

TSAN_INTERCEPTOR(void*, memmove, void *dst, void *src, uptr n) {
  if (!COMMON_INTERCEPTOR_NOTHING_IS_INITIALIZED) {
    SCOPED_TSAN_INTERCEPTOR(memmove, dst, src, n);
    MemoryAccessRange(thr, pc, (uptr)dst, n, true);
    MemoryAccessRange(thr, pc, (uptr)src, n, false);
  }
  return REAL(memmove)(dst, src, n);
}

TSAN_INTERCEPTOR(char*, strchr, char *s, int c) {
  SCOPED_TSAN_INTERCEPTOR(strchr, s, c);
  char *res = REAL(strchr)(s, c);
  uptr len = internal_strlen(s);
  uptr n = res ? (char*)res - (char*)s + 1 : len + 1;
  READ_STRING_OF_LEN(thr, pc, s, len, n);
  return res;
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(char*, strchrnul, char *s, int c) {
  SCOPED_TSAN_INTERCEPTOR(strchrnul, s, c);
  char *res = REAL(strchrnul)(s, c);
  uptr len = (char*)res - (char*)s + 1;
  READ_STRING(thr, pc, s, len);
  return res;
}
#endif

TSAN_INTERCEPTOR(char*, strrchr, char *s, int c) {
  SCOPED_TSAN_INTERCEPTOR(strrchr, s, c);
  MemoryAccessRange(thr, pc, (uptr)s, internal_strlen(s) + 1, false);
  return REAL(strrchr)(s, c);
}

TSAN_INTERCEPTOR(char*, strcpy, char *dst, const char *src) {  // NOLINT
  SCOPED_TSAN_INTERCEPTOR(strcpy, dst, src);  // NOLINT
  uptr srclen = internal_strlen(src);
  MemoryAccessRange(thr, pc, (uptr)dst, srclen + 1, true);
  MemoryAccessRange(thr, pc, (uptr)src, srclen + 1, false);
  return REAL(strcpy)(dst, src);  // NOLINT
}

TSAN_INTERCEPTOR(char*, strncpy, char *dst, char *src, uptr n) {
  SCOPED_TSAN_INTERCEPTOR(strncpy, dst, src, n);
  uptr srclen = internal_strnlen(src, n);
  MemoryAccessRange(thr, pc, (uptr)dst, n, true);
  MemoryAccessRange(thr, pc, (uptr)src, min(srclen + 1, n), false);
  return REAL(strncpy)(dst, src, n);
}

TSAN_INTERCEPTOR(char*, strdup, const char *str) {
  SCOPED_TSAN_INTERCEPTOR(strdup, str);
  // strdup will call malloc, so no instrumentation is required here.
  return REAL(strdup)(str);
}

static bool fix_mmap_addr(void **addr, long_t sz, int flags) {
  if (*addr) {
    if (!IsAppMem((uptr)*addr) || !IsAppMem((uptr)*addr + sz - 1)) {
      if (flags & MAP_FIXED) {
        errno = EINVAL;
        return false;
      } else {
        *addr = 0;
      }
    }
  }
  return true;
}

TSAN_INTERCEPTOR(void *, mmap, void *addr, SIZE_T sz, int prot, int flags,
                 int fd, OFF_T off) {
  SCOPED_TSAN_INTERCEPTOR(mmap, addr, sz, prot, flags, fd, off);
  if (!fix_mmap_addr(&addr, sz, flags))
    return MAP_FAILED;
  void *res = REAL(mmap)(addr, sz, prot, flags, fd, off);
  if (res != MAP_FAILED) {
    if (fd > 0)
      FdAccess(thr, pc, fd);
    MemoryRangeImitateWrite(thr, pc, (uptr)res, sz);
  }
  return res;
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(void *, mmap64, void *addr, SIZE_T sz, int prot, int flags,
                 int fd, OFF64_T off) {
  SCOPED_TSAN_INTERCEPTOR(mmap64, addr, sz, prot, flags, fd, off);
  if (!fix_mmap_addr(&addr, sz, flags))
    return MAP_FAILED;
  void *res = REAL(mmap64)(addr, sz, prot, flags, fd, off);
  if (res != MAP_FAILED) {
    if (fd > 0)
      FdAccess(thr, pc, fd);
    MemoryRangeImitateWrite(thr, pc, (uptr)res, sz);
  }
  return res;
}
#define TSAN_MAYBE_INTERCEPT_MMAP64 TSAN_INTERCEPT(mmap64)
#else
#define TSAN_MAYBE_INTERCEPT_MMAP64
#endif

TSAN_INTERCEPTOR(int, munmap, void *addr, long_t sz) {
  SCOPED_TSAN_INTERCEPTOR(munmap, addr, sz);
  if (sz != 0) {
    // If sz == 0, munmap will return EINVAL and don't unmap any memory.
    DontNeedShadowFor((uptr)addr, sz);
    ctx->metamap.ResetRange(thr, pc, (uptr)addr, (uptr)sz);
  }
  int res = REAL(munmap)(addr, sz);
  return res;
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(void*, memalign, uptr align, uptr sz) {
  SCOPED_INTERCEPTOR_RAW(memalign, align, sz);
  return user_alloc(thr, pc, sz, align);
}
#define TSAN_MAYBE_INTERCEPT_MEMALIGN TSAN_INTERCEPT(memalign)
#else
#define TSAN_MAYBE_INTERCEPT_MEMALIGN
#endif

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(void*, aligned_alloc, uptr align, uptr sz) {
  SCOPED_INTERCEPTOR_RAW(memalign, align, sz);
  return user_alloc(thr, pc, sz, align);
}

TSAN_INTERCEPTOR(void*, valloc, uptr sz) {
  SCOPED_INTERCEPTOR_RAW(valloc, sz);
  return user_alloc(thr, pc, sz, GetPageSizeCached());
}
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(void*, pvalloc, uptr sz) {
  SCOPED_INTERCEPTOR_RAW(pvalloc, sz);
  sz = RoundUp(sz, GetPageSizeCached());
  return user_alloc(thr, pc, sz, GetPageSizeCached());
}
#define TSAN_MAYBE_INTERCEPT_PVALLOC TSAN_INTERCEPT(pvalloc)
#else
#define TSAN_MAYBE_INTERCEPT_PVALLOC
#endif

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, posix_memalign, void **memptr, uptr align, uptr sz) {
  SCOPED_INTERCEPTOR_RAW(posix_memalign, memptr, align, sz);
  *memptr = user_alloc(thr, pc, sz, align);
  return 0;
}
#endif

// Used in thread-safe function static initialization.
extern "C" int INTERFACE_ATTRIBUTE __cxa_guard_acquire(atomic_uint32_t *g) {
  SCOPED_INTERCEPTOR_RAW(__cxa_guard_acquire, g);
  for (;;) {
    u32 cmp = atomic_load(g, memory_order_acquire);
    if (cmp == 0) {
      if (atomic_compare_exchange_strong(g, &cmp, 1<<16, memory_order_relaxed))
        return 1;
    } else if (cmp == 1) {
      Acquire(thr, pc, (uptr)g);
      return 0;
    } else {
      internal_sched_yield();
    }
  }
}

extern "C" void INTERFACE_ATTRIBUTE __cxa_guard_release(atomic_uint32_t *g) {
  SCOPED_INTERCEPTOR_RAW(__cxa_guard_release, g);
  Release(thr, pc, (uptr)g);
  atomic_store(g, 1, memory_order_release);
}

extern "C" void INTERFACE_ATTRIBUTE __cxa_guard_abort(atomic_uint32_t *g) {
  SCOPED_INTERCEPTOR_RAW(__cxa_guard_abort, g);
  atomic_store(g, 0, memory_order_relaxed);
}

namespace __tsan {
void DestroyThreadState() {
  ThreadState *thr = cur_thread();
  ThreadFinish(thr);
  ThreadSignalContext *sctx = thr->signal_ctx;
  if (sctx) {
    thr->signal_ctx = 0;
    UnmapOrDie(sctx, sizeof(*sctx));
  }
  cur_thread_finalize();
}
}  // namespace __tsan

static void thread_finalize(void *v) {
  uptr iter = (uptr)v;
  if (iter > 1) {
    if (pthread_setspecific(g_thread_finalize_key, (void*)(iter - 1))) {
      Printf("ThreadSanitizer: failed to set thread key\n");
      Die();
    }
    return;
  }
  DestroyThreadState();
}


struct ThreadParam {
  void* (*callback)(void *arg);
  void *param;
  atomic_uintptr_t tid;
};

extern "C" void *__tsan_thread_start_func(void *arg) {
  ThreadParam *p = (ThreadParam*)arg;
  void* (*callback)(void *arg) = p->callback;
  void *param = p->param;
  int tid = 0;
  {
    ThreadState *thr = cur_thread();
    // Thread-local state is not initialized yet.
    ScopedIgnoreInterceptors ignore;
    ThreadIgnoreBegin(thr, 0);
    if (pthread_setspecific(g_thread_finalize_key,
                            (void *)GetPthreadDestructorIterations())) {
      Printf("ThreadSanitizer: failed to set thread key\n");
      Die();
    }
    ThreadIgnoreEnd(thr, 0);
    while ((tid = atomic_load(&p->tid, memory_order_acquire)) == 0)
      internal_sched_yield();
    ThreadStart(thr, tid, GetTid());
    atomic_store(&p->tid, 0, memory_order_release);
  }
  void *res = callback(param);
  // Prevent the callback from being tail called,
  // it mixes up stack traces.
  volatile int foo = 42;
  foo++;
  return res;
}

TSAN_INTERCEPTOR(int, pthread_create,
    void *th, void *attr, void *(*callback)(void*), void * param) {
  SCOPED_INTERCEPTOR_RAW(pthread_create, th, attr, callback, param);
  if (ctx->after_multithreaded_fork) {
    if (flags()->die_after_fork) {
      Report("ThreadSanitizer: starting new threads after multi-threaded "
          "fork is not supported. Dying (set die_after_fork=0 to override)\n");
      Die();
    } else {
      VPrintf(1, "ThreadSanitizer: starting new threads after multi-threaded "
          "fork is not supported (pid %d). Continuing because of "
          "die_after_fork=0, but you are on your own\n", internal_getpid());
    }
  }
  __sanitizer_pthread_attr_t myattr;
  if (attr == 0) {
    pthread_attr_init(&myattr);
    attr = &myattr;
  }
  int detached = 0;
  REAL(pthread_attr_getdetachstate)(attr, &detached);
  AdjustStackSize(attr);

  ThreadParam p;
  p.callback = callback;
  p.param = param;
  atomic_store(&p.tid, 0, memory_order_relaxed);
  int res = -1;
  {
    // Otherwise we see false positives in pthread stack manipulation.
    ScopedIgnoreInterceptors ignore;
    ThreadIgnoreBegin(thr, pc);
    res = REAL(pthread_create)(th, attr, __tsan_thread_start_func, &p);
    ThreadIgnoreEnd(thr, pc);
  }
  if (res == 0) {
    int tid = ThreadCreate(thr, pc, *(uptr*)th,
                           detached == PTHREAD_CREATE_DETACHED);
    CHECK_NE(tid, 0);
    // Synchronization on p.tid serves two purposes:
    // 1. ThreadCreate must finish before the new thread starts.
    //    Otherwise the new thread can call pthread_detach, but the pthread_t
    //    identifier is not yet registered in ThreadRegistry by ThreadCreate.
    // 2. ThreadStart must finish before this thread continues.
    //    Otherwise, this thread can call pthread_detach and reset thr->sync
    //    before the new thread got a chance to acquire from it in ThreadStart.
    atomic_store(&p.tid, tid, memory_order_release);
    while (atomic_load(&p.tid, memory_order_acquire) != 0)
      internal_sched_yield();
  }
  if (attr == &myattr)
    pthread_attr_destroy(&myattr);
  return res;
}

TSAN_INTERCEPTOR(int, pthread_join, void *th, void **ret) {
  SCOPED_INTERCEPTOR_RAW(pthread_join, th, ret);
  int tid = ThreadTid(thr, pc, (uptr)th);
  ThreadIgnoreBegin(thr, pc);
  int res = BLOCK_REAL(pthread_join)(th, ret);
  ThreadIgnoreEnd(thr, pc);
  if (res == 0) {
    ThreadJoin(thr, pc, tid);
  }
  return res;
}

DEFINE_REAL_PTHREAD_FUNCTIONS

TSAN_INTERCEPTOR(int, pthread_detach, void *th) {
  SCOPED_TSAN_INTERCEPTOR(pthread_detach, th);
  int tid = ThreadTid(thr, pc, (uptr)th);
  int res = REAL(pthread_detach)(th);
  if (res == 0) {
    ThreadDetach(thr, pc, tid);
  }
  return res;
}

// Problem:
// NPTL implementation of pthread_cond has 2 versions (2.2.5 and 2.3.2).
// pthread_cond_t has different size in the different versions.
// If call new REAL functions for old pthread_cond_t, they will corrupt memory
// after pthread_cond_t (old cond is smaller).
// If we call old REAL functions for new pthread_cond_t, we will lose  some
// functionality (e.g. old functions do not support waiting against
// CLOCK_REALTIME).
// Proper handling would require to have 2 versions of interceptors as well.
// But this is messy, in particular requires linker scripts when sanitizer
// runtime is linked into a shared library.
// Instead we assume we don't have dynamic libraries built against old
// pthread (2.2.5 is dated by 2002). And provide legacy_pthread_cond flag
// that allows to work with old libraries (but this mode does not support
// some features, e.g. pthread_condattr_getpshared).
static void *init_cond(void *c, bool force = false) {
  // sizeof(pthread_cond_t) >= sizeof(uptr) in both versions.
  // So we allocate additional memory on the side large enough to hold
  // any pthread_cond_t object. Always call new REAL functions, but pass
  // the aux object to them.
  // Note: the code assumes that PTHREAD_COND_INITIALIZER initializes
  // first word of pthread_cond_t to zero.
  // It's all relevant only for linux.
  if (!common_flags()->legacy_pthread_cond)
    return c;
  atomic_uintptr_t *p = (atomic_uintptr_t*)c;
  uptr cond = atomic_load(p, memory_order_acquire);
  if (!force && cond != 0)
    return (void*)cond;
  void *newcond = WRAP(malloc)(pthread_cond_t_sz);
  internal_memset(newcond, 0, pthread_cond_t_sz);
  if (atomic_compare_exchange_strong(p, &cond, (uptr)newcond,
      memory_order_acq_rel))
    return newcond;
  WRAP(free)(newcond);
  return (void*)cond;
}

struct CondMutexUnlockCtx {
  ScopedInterceptor *si;
  ThreadState *thr;
  uptr pc;
  void *m;
};

static void cond_mutex_unlock(CondMutexUnlockCtx *arg) {
  // pthread_cond_wait interceptor has enabled async signal delivery
  // (see BlockingCall below). Disable async signals since we are running
  // tsan code. Also ScopedInterceptor and BlockingCall destructors won't run
  // since the thread is cancelled, so we have to manually execute them
  // (the thread still can run some user code due to pthread_cleanup_push).
  ThreadSignalContext *ctx = SigCtx(arg->thr);
  CHECK_EQ(atomic_load(&ctx->in_blocking_func, memory_order_relaxed), 1);
  atomic_store(&ctx->in_blocking_func, 0, memory_order_relaxed);
  MutexLock(arg->thr, arg->pc, (uptr)arg->m);
  // Undo BlockingCall ctor effects.
  arg->thr->ignore_interceptors--;
  arg->si->~ScopedInterceptor();
}

INTERCEPTOR(int, pthread_cond_init, void *c, void *a) {
  void *cond = init_cond(c, true);
  SCOPED_TSAN_INTERCEPTOR(pthread_cond_init, cond, a);
  MemoryAccessRange(thr, pc, (uptr)c, sizeof(uptr), true);
  return REAL(pthread_cond_init)(cond, a);
}

INTERCEPTOR(int, pthread_cond_wait, void *c, void *m) {
  void *cond = init_cond(c);
  SCOPED_TSAN_INTERCEPTOR(pthread_cond_wait, cond, m);
  MemoryAccessRange(thr, pc, (uptr)c, sizeof(uptr), false);
  MutexUnlock(thr, pc, (uptr)m);
  CondMutexUnlockCtx arg = {&si, thr, pc, m};
  int res = 0;
  // This ensures that we handle mutex lock even in case of pthread_cancel.
  // See test/tsan/cond_cancel.cc.
  {
    // Enable signal delivery while the thread is blocked.
    BlockingCall bc(thr);
    res = call_pthread_cancel_with_cleanup(
        (int(*)(void *c, void *m, void *abstime))REAL(pthread_cond_wait),
        cond, m, 0, (void(*)(void *arg))cond_mutex_unlock, &arg);
  }
  if (res == errno_EOWNERDEAD)
    MutexRepair(thr, pc, (uptr)m);
  MutexLock(thr, pc, (uptr)m);
  return res;
}

INTERCEPTOR(int, pthread_cond_timedwait, void *c, void *m, void *abstime) {
  void *cond = init_cond(c);
  SCOPED_TSAN_INTERCEPTOR(pthread_cond_timedwait, cond, m, abstime);
  MemoryAccessRange(thr, pc, (uptr)c, sizeof(uptr), false);
  MutexUnlock(thr, pc, (uptr)m);
  CondMutexUnlockCtx arg = {&si, thr, pc, m};
  int res = 0;
  // This ensures that we handle mutex lock even in case of pthread_cancel.
  // See test/tsan/cond_cancel.cc.
  {
    BlockingCall bc(thr);
    res = call_pthread_cancel_with_cleanup(
        REAL(pthread_cond_timedwait), cond, m, abstime,
        (void(*)(void *arg))cond_mutex_unlock, &arg);
  }
  if (res == errno_EOWNERDEAD)
    MutexRepair(thr, pc, (uptr)m);
  MutexLock(thr, pc, (uptr)m);
  return res;
}

INTERCEPTOR(int, pthread_cond_signal, void *c) {
  void *cond = init_cond(c);
  SCOPED_TSAN_INTERCEPTOR(pthread_cond_signal, cond);
  MemoryAccessRange(thr, pc, (uptr)c, sizeof(uptr), false);
  return REAL(pthread_cond_signal)(cond);
}

INTERCEPTOR(int, pthread_cond_broadcast, void *c) {
  void *cond = init_cond(c);
  SCOPED_TSAN_INTERCEPTOR(pthread_cond_broadcast, cond);
  MemoryAccessRange(thr, pc, (uptr)c, sizeof(uptr), false);
  return REAL(pthread_cond_broadcast)(cond);
}

INTERCEPTOR(int, pthread_cond_destroy, void *c) {
  void *cond = init_cond(c);
  SCOPED_TSAN_INTERCEPTOR(pthread_cond_destroy, cond);
  MemoryAccessRange(thr, pc, (uptr)c, sizeof(uptr), true);
  int res = REAL(pthread_cond_destroy)(cond);
  if (common_flags()->legacy_pthread_cond) {
    // Free our aux cond and zero the pointer to not leave dangling pointers.
    WRAP(free)(cond);
    atomic_store((atomic_uintptr_t*)c, 0, memory_order_relaxed);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_mutex_init, void *m, void *a) {
  SCOPED_TSAN_INTERCEPTOR(pthread_mutex_init, m, a);
  int res = REAL(pthread_mutex_init)(m, a);
  if (res == 0) {
    bool recursive = false;
    if (a) {
      int type = 0;
      if (REAL(pthread_mutexattr_gettype)(a, &type) == 0)
        recursive = (type == PTHREAD_MUTEX_RECURSIVE
            || type == PTHREAD_MUTEX_RECURSIVE_NP);
    }
    MutexCreate(thr, pc, (uptr)m, false, recursive, false);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_mutex_destroy, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_mutex_destroy, m);
  int res = REAL(pthread_mutex_destroy)(m);
  if (res == 0 || res == EBUSY) {
    MutexDestroy(thr, pc, (uptr)m);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_mutex_trylock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_mutex_trylock, m);
  int res = REAL(pthread_mutex_trylock)(m);
  if (res == EOWNERDEAD)
    MutexRepair(thr, pc, (uptr)m);
  if (res == 0 || res == EOWNERDEAD)
    MutexLock(thr, pc, (uptr)m, /*rec=*/1, /*try_lock=*/true);
  return res;
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, pthread_mutex_timedlock, void *m, void *abstime) {
  SCOPED_TSAN_INTERCEPTOR(pthread_mutex_timedlock, m, abstime);
  int res = REAL(pthread_mutex_timedlock)(m, abstime);
  if (res == 0) {
    MutexLock(thr, pc, (uptr)m);
  }
  return res;
}
#endif

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, pthread_spin_init, void *m, int pshared) {
  SCOPED_TSAN_INTERCEPTOR(pthread_spin_init, m, pshared);
  int res = REAL(pthread_spin_init)(m, pshared);
  if (res == 0) {
    MutexCreate(thr, pc, (uptr)m, false, false, false);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_spin_destroy, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_spin_destroy, m);
  int res = REAL(pthread_spin_destroy)(m);
  if (res == 0) {
    MutexDestroy(thr, pc, (uptr)m);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_spin_lock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_spin_lock, m);
  int res = REAL(pthread_spin_lock)(m);
  if (res == 0) {
    MutexLock(thr, pc, (uptr)m);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_spin_trylock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_spin_trylock, m);
  int res = REAL(pthread_spin_trylock)(m);
  if (res == 0) {
    MutexLock(thr, pc, (uptr)m, /*rec=*/1, /*try_lock=*/true);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_spin_unlock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_spin_unlock, m);
  MutexUnlock(thr, pc, (uptr)m);
  int res = REAL(pthread_spin_unlock)(m);
  return res;
}
#endif

TSAN_INTERCEPTOR(int, pthread_rwlock_init, void *m, void *a) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_init, m, a);
  int res = REAL(pthread_rwlock_init)(m, a);
  if (res == 0) {
    MutexCreate(thr, pc, (uptr)m, true, false, false);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_rwlock_destroy, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_destroy, m);
  int res = REAL(pthread_rwlock_destroy)(m);
  if (res == 0) {
    MutexDestroy(thr, pc, (uptr)m);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_rwlock_rdlock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_rdlock, m);
  int res = REAL(pthread_rwlock_rdlock)(m);
  if (res == 0) {
    MutexReadLock(thr, pc, (uptr)m);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_rwlock_tryrdlock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_tryrdlock, m);
  int res = REAL(pthread_rwlock_tryrdlock)(m);
  if (res == 0) {
    MutexReadLock(thr, pc, (uptr)m, /*try_lock=*/true);
  }
  return res;
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, pthread_rwlock_timedrdlock, void *m, void *abstime) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_timedrdlock, m, abstime);
  int res = REAL(pthread_rwlock_timedrdlock)(m, abstime);
  if (res == 0) {
    MutexReadLock(thr, pc, (uptr)m);
  }
  return res;
}
#endif

TSAN_INTERCEPTOR(int, pthread_rwlock_wrlock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_wrlock, m);
  int res = REAL(pthread_rwlock_wrlock)(m);
  if (res == 0) {
    MutexLock(thr, pc, (uptr)m);
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_rwlock_trywrlock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_trywrlock, m);
  int res = REAL(pthread_rwlock_trywrlock)(m);
  if (res == 0) {
    MutexLock(thr, pc, (uptr)m, /*rec=*/1, /*try_lock=*/true);
  }
  return res;
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, pthread_rwlock_timedwrlock, void *m, void *abstime) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_timedwrlock, m, abstime);
  int res = REAL(pthread_rwlock_timedwrlock)(m, abstime);
  if (res == 0) {
    MutexLock(thr, pc, (uptr)m);
  }
  return res;
}
#endif

TSAN_INTERCEPTOR(int, pthread_rwlock_unlock, void *m) {
  SCOPED_TSAN_INTERCEPTOR(pthread_rwlock_unlock, m);
  MutexReadOrWriteUnlock(thr, pc, (uptr)m);
  int res = REAL(pthread_rwlock_unlock)(m);
  return res;
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, pthread_barrier_init, void *b, void *a, unsigned count) {
  SCOPED_TSAN_INTERCEPTOR(pthread_barrier_init, b, a, count);
  MemoryWrite(thr, pc, (uptr)b, kSizeLog1);
  int res = REAL(pthread_barrier_init)(b, a, count);
  return res;
}

TSAN_INTERCEPTOR(int, pthread_barrier_destroy, void *b) {
  SCOPED_TSAN_INTERCEPTOR(pthread_barrier_destroy, b);
  MemoryWrite(thr, pc, (uptr)b, kSizeLog1);
  int res = REAL(pthread_barrier_destroy)(b);
  return res;
}

TSAN_INTERCEPTOR(int, pthread_barrier_wait, void *b) {
  SCOPED_TSAN_INTERCEPTOR(pthread_barrier_wait, b);
  Release(thr, pc, (uptr)b);
  MemoryRead(thr, pc, (uptr)b, kSizeLog1);
  int res = REAL(pthread_barrier_wait)(b);
  MemoryRead(thr, pc, (uptr)b, kSizeLog1);
  if (res == 0 || res == PTHREAD_BARRIER_SERIAL_THREAD) {
    Acquire(thr, pc, (uptr)b);
  }
  return res;
}
#endif

TSAN_INTERCEPTOR(int, pthread_once, void *o, void (*f)()) {
  SCOPED_INTERCEPTOR_RAW(pthread_once, o, f);
  if (o == 0 || f == 0)
    return EINVAL;
  atomic_uint32_t *a;
  if (!SANITIZER_MAC)
    a = static_cast<atomic_uint32_t*>(o);
  else  // On OS X, pthread_once_t has a header with a long-sized signature.
    a = static_cast<atomic_uint32_t*>((void *)((char *)o + sizeof(long_t)));
  u32 v = atomic_load(a, memory_order_acquire);
  if (v == 0 && atomic_compare_exchange_strong(a, &v, 1,
                                               memory_order_relaxed)) {
    (*f)();
    if (!thr->in_ignored_lib)
      Release(thr, pc, (uptr)o);
    atomic_store(a, 2, memory_order_release);
  } else {
    while (v != 2) {
      internal_sched_yield();
      v = atomic_load(a, memory_order_acquire);
    }
    if (!thr->in_ignored_lib)
      Acquire(thr, pc, (uptr)o);
  }
  return 0;
}

TSAN_INTERCEPTOR(int, sem_init, void *s, int pshared, unsigned value) {
  SCOPED_TSAN_INTERCEPTOR(sem_init, s, pshared, value);
  int res = REAL(sem_init)(s, pshared, value);
  return res;
}

TSAN_INTERCEPTOR(int, sem_destroy, void *s) {
  SCOPED_TSAN_INTERCEPTOR(sem_destroy, s);
  int res = REAL(sem_destroy)(s);
  return res;
}

TSAN_INTERCEPTOR(int, sem_wait, void *s) {
  SCOPED_TSAN_INTERCEPTOR(sem_wait, s);
  int res = BLOCK_REAL(sem_wait)(s);
  if (res == 0) {
    Acquire(thr, pc, (uptr)s);
  }
  return res;
}

TSAN_INTERCEPTOR(int, sem_trywait, void *s) {
  SCOPED_TSAN_INTERCEPTOR(sem_trywait, s);
  int res = BLOCK_REAL(sem_trywait)(s);
  if (res == 0) {
    Acquire(thr, pc, (uptr)s);
  }
  return res;
}

TSAN_INTERCEPTOR(int, sem_timedwait, void *s, void *abstime) {
  SCOPED_TSAN_INTERCEPTOR(sem_timedwait, s, abstime);
  int res = BLOCK_REAL(sem_timedwait)(s, abstime);
  if (res == 0) {
    Acquire(thr, pc, (uptr)s);
  }
  return res;
}

TSAN_INTERCEPTOR(int, sem_post, void *s) {
  SCOPED_TSAN_INTERCEPTOR(sem_post, s);
  Release(thr, pc, (uptr)s);
  int res = REAL(sem_post)(s);
  return res;
}

TSAN_INTERCEPTOR(int, sem_getvalue, void *s, int *sval) {
  SCOPED_TSAN_INTERCEPTOR(sem_getvalue, s, sval);
  int res = REAL(sem_getvalue)(s, sval);
  if (res == 0) {
    Acquire(thr, pc, (uptr)s);
  }
  return res;
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, __xstat, int version, const char *path, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__xstat, version, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__xstat)(version, path, buf);
}
#define TSAN_MAYBE_INTERCEPT___XSTAT TSAN_INTERCEPT(__xstat)
#else
#define TSAN_MAYBE_INTERCEPT___XSTAT
#endif

TSAN_INTERCEPTOR(int, stat, const char *path, void *buf) {
#if SANITIZER_FREEBSD || SANITIZER_NETBSD || SANITIZER_MAC
  SCOPED_TSAN_INTERCEPTOR(stat, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(stat)(path, buf);
#else
  SCOPED_TSAN_INTERCEPTOR(__xstat, 0, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__xstat)(0, path, buf);
#endif
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, __xstat64, int version, const char *path, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__xstat64, version, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__xstat64)(version, path, buf);
}
#define TSAN_MAYBE_INTERCEPT___XSTAT64 TSAN_INTERCEPT(__xstat64)
#else
#define TSAN_MAYBE_INTERCEPT___XSTAT64
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, stat64, const char *path, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__xstat64, 0, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__xstat64)(0, path, buf);
}
#define TSAN_MAYBE_INTERCEPT_STAT64 TSAN_INTERCEPT(stat64)
#else
#define TSAN_MAYBE_INTERCEPT_STAT64
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, __lxstat, int version, const char *path, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__lxstat, version, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__lxstat)(version, path, buf);
}
#define TSAN_MAYBE_INTERCEPT___LXSTAT TSAN_INTERCEPT(__lxstat)
#else
#define TSAN_MAYBE_INTERCEPT___LXSTAT
#endif

TSAN_INTERCEPTOR(int, lstat, const char *path, void *buf) {
#if SANITIZER_FREEBSD || SANITIZER_NETBSD || SANITIZER_MAC
  SCOPED_TSAN_INTERCEPTOR(lstat, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(lstat)(path, buf);
#else
  SCOPED_TSAN_INTERCEPTOR(__lxstat, 0, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__lxstat)(0, path, buf);
#endif
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, __lxstat64, int version, const char *path, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__lxstat64, version, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__lxstat64)(version, path, buf);
}
#define TSAN_MAYBE_INTERCEPT___LXSTAT64 TSAN_INTERCEPT(__lxstat64)
#else
#define TSAN_MAYBE_INTERCEPT___LXSTAT64
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, lstat64, const char *path, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__lxstat64, 0, path, buf);
  READ_STRING(thr, pc, path, 0);
  return REAL(__lxstat64)(0, path, buf);
}
#define TSAN_MAYBE_INTERCEPT_LSTAT64 TSAN_INTERCEPT(lstat64)
#else
#define TSAN_MAYBE_INTERCEPT_LSTAT64
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, __fxstat, int version, int fd, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__fxstat, version, fd, buf);
  if (fd > 0)
    FdAccess(thr, pc, fd);
  return REAL(__fxstat)(version, fd, buf);
}
#define TSAN_MAYBE_INTERCEPT___FXSTAT TSAN_INTERCEPT(__fxstat)
#else
#define TSAN_MAYBE_INTERCEPT___FXSTAT
#endif

TSAN_INTERCEPTOR(int, fstat, int fd, void *buf) {
#if SANITIZER_FREEBSD || SANITIZER_NETBSD || SANITIZER_MAC
  SCOPED_TSAN_INTERCEPTOR(fstat, fd, buf);
  if (fd > 0)
    FdAccess(thr, pc, fd);
  return REAL(fstat)(fd, buf);
#else
  SCOPED_TSAN_INTERCEPTOR(__fxstat, 0, fd, buf);
  if (fd > 0)
    FdAccess(thr, pc, fd);
  return REAL(__fxstat)(0, fd, buf);
#endif
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, __fxstat64, int version, int fd, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__fxstat64, version, fd, buf);
  if (fd > 0)
    FdAccess(thr, pc, fd);
  return REAL(__fxstat64)(version, fd, buf);
}
#define TSAN_MAYBE_INTERCEPT___FXSTAT64 TSAN_INTERCEPT(__fxstat64)
#else
#define TSAN_MAYBE_INTERCEPT___FXSTAT64
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, fstat64, int fd, void *buf) {
  SCOPED_TSAN_INTERCEPTOR(__fxstat64, 0, fd, buf);
  if (fd > 0)
    FdAccess(thr, pc, fd);
  return REAL(__fxstat64)(0, fd, buf);
}
#define TSAN_MAYBE_INTERCEPT_FSTAT64 TSAN_INTERCEPT(fstat64)
#else
#define TSAN_MAYBE_INTERCEPT_FSTAT64
#endif

TSAN_INTERCEPTOR(int, open, const char *name, int flags, int mode) {
  SCOPED_TSAN_INTERCEPTOR(open, name, flags, mode);
  READ_STRING(thr, pc, name, 0);
  int fd = REAL(open)(name, flags, mode);
  if (fd >= 0)
    FdFileCreate(thr, pc, fd);
  return fd;
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, open64, const char *name, int flags, int mode) {
  SCOPED_TSAN_INTERCEPTOR(open64, name, flags, mode);
  READ_STRING(thr, pc, name, 0);
  int fd = REAL(open64)(name, flags, mode);
  if (fd >= 0)
    FdFileCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_OPEN64 TSAN_INTERCEPT(open64)
#else
#define TSAN_MAYBE_INTERCEPT_OPEN64
#endif

TSAN_INTERCEPTOR(int, creat, const char *name, int mode) {
  SCOPED_TSAN_INTERCEPTOR(creat, name, mode);
  READ_STRING(thr, pc, name, 0);
  int fd = REAL(creat)(name, mode);
  if (fd >= 0)
    FdFileCreate(thr, pc, fd);
  return fd;
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, creat64, const char *name, int mode) {
  SCOPED_TSAN_INTERCEPTOR(creat64, name, mode);
  READ_STRING(thr, pc, name, 0);
  int fd = REAL(creat64)(name, mode);
  if (fd >= 0)
    FdFileCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_CREAT64 TSAN_INTERCEPT(creat64)
#else
#define TSAN_MAYBE_INTERCEPT_CREAT64
#endif

TSAN_INTERCEPTOR(int, dup, int oldfd) {
  SCOPED_TSAN_INTERCEPTOR(dup, oldfd);
  int newfd = REAL(dup)(oldfd);
  if (oldfd >= 0 && newfd >= 0 && newfd != oldfd)
    FdDup(thr, pc, oldfd, newfd, true);
  return newfd;
}

TSAN_INTERCEPTOR(int, dup2, int oldfd, int newfd) {
  SCOPED_TSAN_INTERCEPTOR(dup2, oldfd, newfd);
  int newfd2 = REAL(dup2)(oldfd, newfd);
  if (oldfd >= 0 && newfd2 >= 0 && newfd2 != oldfd)
    FdDup(thr, pc, oldfd, newfd2, false);
  return newfd2;
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, dup3, int oldfd, int newfd, int flags) {
  SCOPED_TSAN_INTERCEPTOR(dup3, oldfd, newfd, flags);
  int newfd2 = REAL(dup3)(oldfd, newfd, flags);
  if (oldfd >= 0 && newfd2 >= 0 && newfd2 != oldfd)
    FdDup(thr, pc, oldfd, newfd2, false);
  return newfd2;
}
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, eventfd, unsigned initval, int flags) {
  SCOPED_TSAN_INTERCEPTOR(eventfd, initval, flags);
  int fd = REAL(eventfd)(initval, flags);
  if (fd >= 0)
    FdEventCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_EVENTFD TSAN_INTERCEPT(eventfd)
#else
#define TSAN_MAYBE_INTERCEPT_EVENTFD
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, signalfd, int fd, void *mask, int flags) {
  SCOPED_TSAN_INTERCEPTOR(signalfd, fd, mask, flags);
  if (fd >= 0)
    FdClose(thr, pc, fd);
  fd = REAL(signalfd)(fd, mask, flags);
  if (fd >= 0)
    FdSignalCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_SIGNALFD TSAN_INTERCEPT(signalfd)
#else
#define TSAN_MAYBE_INTERCEPT_SIGNALFD
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, inotify_init, int fake) {
  SCOPED_TSAN_INTERCEPTOR(inotify_init, fake);
  int fd = REAL(inotify_init)(fake);
  if (fd >= 0)
    FdInotifyCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_INOTIFY_INIT TSAN_INTERCEPT(inotify_init)
#else
#define TSAN_MAYBE_INTERCEPT_INOTIFY_INIT
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, inotify_init1, int flags) {
  SCOPED_TSAN_INTERCEPTOR(inotify_init1, flags);
  int fd = REAL(inotify_init1)(flags);
  if (fd >= 0)
    FdInotifyCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_INOTIFY_INIT1 TSAN_INTERCEPT(inotify_init1)
#else
#define TSAN_MAYBE_INTERCEPT_INOTIFY_INIT1
#endif

TSAN_INTERCEPTOR(int, socket, int domain, int type, int protocol) {
  SCOPED_TSAN_INTERCEPTOR(socket, domain, type, protocol);
  int fd = REAL(socket)(domain, type, protocol);
  if (fd >= 0)
    FdSocketCreate(thr, pc, fd);
  return fd;
}

TSAN_INTERCEPTOR(int, socketpair, int domain, int type, int protocol, int *fd) {
  SCOPED_TSAN_INTERCEPTOR(socketpair, domain, type, protocol, fd);
  int res = REAL(socketpair)(domain, type, protocol, fd);
  if (res == 0 && fd[0] >= 0 && fd[1] >= 0)
    FdPipeCreate(thr, pc, fd[0], fd[1]);
  return res;
}

TSAN_INTERCEPTOR(int, connect, int fd, void *addr, unsigned addrlen) {
  SCOPED_TSAN_INTERCEPTOR(connect, fd, addr, addrlen);
  FdSocketConnecting(thr, pc, fd);
  int res = REAL(connect)(fd, addr, addrlen);
  if (res == 0 && fd >= 0)
    FdSocketConnect(thr, pc, fd);
  return res;
}

TSAN_INTERCEPTOR(int, bind, int fd, void *addr, unsigned addrlen) {
  SCOPED_TSAN_INTERCEPTOR(bind, fd, addr, addrlen);
  int res = REAL(bind)(fd, addr, addrlen);
  if (fd > 0 && res == 0)
    FdAccess(thr, pc, fd);
  return res;
}

TSAN_INTERCEPTOR(int, listen, int fd, int backlog) {
  SCOPED_TSAN_INTERCEPTOR(listen, fd, backlog);
  int res = REAL(listen)(fd, backlog);
  if (fd > 0 && res == 0)
    FdAccess(thr, pc, fd);
  return res;
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, epoll_create, int size) {
  SCOPED_TSAN_INTERCEPTOR(epoll_create, size);
  int fd = REAL(epoll_create)(size);
  if (fd >= 0)
    FdPollCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_EPOLL_CREATE TSAN_INTERCEPT(epoll_create)
#else
#define TSAN_MAYBE_INTERCEPT_EPOLL_CREATE
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, epoll_create1, int flags) {
  SCOPED_TSAN_INTERCEPTOR(epoll_create1, flags);
  int fd = REAL(epoll_create1)(flags);
  if (fd >= 0)
    FdPollCreate(thr, pc, fd);
  return fd;
}
#define TSAN_MAYBE_INTERCEPT_EPOLL_CREATE1 TSAN_INTERCEPT(epoll_create1)
#else
#define TSAN_MAYBE_INTERCEPT_EPOLL_CREATE1
#endif

TSAN_INTERCEPTOR(int, close, int fd) {
  SCOPED_TSAN_INTERCEPTOR(close, fd);
  if (fd >= 0)
    FdClose(thr, pc, fd);
  return REAL(close)(fd);
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, __close, int fd) {
  SCOPED_TSAN_INTERCEPTOR(__close, fd);
  if (fd >= 0)
    FdClose(thr, pc, fd);
  return REAL(__close)(fd);
}
#define TSAN_MAYBE_INTERCEPT___CLOSE TSAN_INTERCEPT(__close)
#else
#define TSAN_MAYBE_INTERCEPT___CLOSE
#endif

// glibc guts
#if SANITIZER_LINUX
TSAN_INTERCEPTOR(void, __res_iclose, void *state, bool free_addr) {
  SCOPED_TSAN_INTERCEPTOR(__res_iclose, state, free_addr);
  int fds[64];
  int cnt = ExtractResolvFDs(state, fds, ARRAY_SIZE(fds));
  for (int i = 0; i < cnt; i++) {
    if (fds[i] > 0)
      FdClose(thr, pc, fds[i]);
  }
  REAL(__res_iclose)(state, free_addr);
}
#define TSAN_MAYBE_INTERCEPT___RES_ICLOSE TSAN_INTERCEPT(__res_iclose)
#else
#define TSAN_MAYBE_INTERCEPT___RES_ICLOSE
#endif

TSAN_INTERCEPTOR(int, pipe, int *pipefd) {
  SCOPED_TSAN_INTERCEPTOR(pipe, pipefd);
  int res = REAL(pipe)(pipefd);
  if (res == 0 && pipefd[0] >= 0 && pipefd[1] >= 0)
    FdPipeCreate(thr, pc, pipefd[0], pipefd[1]);
  return res;
}

#if !SANITIZER_MAC
TSAN_INTERCEPTOR(int, pipe2, int *pipefd, int flags) {
  SCOPED_TSAN_INTERCEPTOR(pipe2, pipefd, flags);
  int res = REAL(pipe2)(pipefd, flags);
  if (res == 0 && pipefd[0] >= 0 && pipefd[1] >= 0)
    FdPipeCreate(thr, pc, pipefd[0], pipefd[1]);
  return res;
}
#endif

TSAN_INTERCEPTOR(long_t, send, int fd, void *buf, long_t len, int flags) {
  SCOPED_TSAN_INTERCEPTOR(send, fd, buf, len, flags);
  if (fd >= 0) {
    FdAccess(thr, pc, fd);
    FdRelease(thr, pc, fd);
  }
  int res = REAL(send)(fd, buf, len, flags);
  return res;
}

TSAN_INTERCEPTOR(long_t, sendmsg, int fd, void *msg, int flags) {
  SCOPED_TSAN_INTERCEPTOR(sendmsg, fd, msg, flags);
  if (fd >= 0) {
    FdAccess(thr, pc, fd);
    FdRelease(thr, pc, fd);
  }
  int res = REAL(sendmsg)(fd, msg, flags);
  return res;
}

TSAN_INTERCEPTOR(long_t, recv, int fd, void *buf, long_t len, int flags) {
  SCOPED_TSAN_INTERCEPTOR(recv, fd, buf, len, flags);
  if (fd >= 0)
    FdAccess(thr, pc, fd);
  int res = REAL(recv)(fd, buf, len, flags);
  if (res >= 0 && fd >= 0) {
    FdAcquire(thr, pc, fd);
  }
  return res;
}

TSAN_INTERCEPTOR(int, unlink, char *path) {
  SCOPED_TSAN_INTERCEPTOR(unlink, path);
  Release(thr, pc, File2addr(path));
  int res = REAL(unlink)(path);
  return res;
}

TSAN_INTERCEPTOR(void*, tmpfile, int fake) {
  SCOPED_TSAN_INTERCEPTOR(tmpfile, fake);
  void *res = REAL(tmpfile)(fake);
  if (res) {
    int fd = fileno_unlocked(res);
    if (fd >= 0)
      FdFileCreate(thr, pc, fd);
  }
  return res;
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(void*, tmpfile64, int fake) {
  SCOPED_TSAN_INTERCEPTOR(tmpfile64, fake);
  void *res = REAL(tmpfile64)(fake);
  if (res) {
    int fd = fileno_unlocked(res);
    if (fd >= 0)
      FdFileCreate(thr, pc, fd);
  }
  return res;
}
#define TSAN_MAYBE_INTERCEPT_TMPFILE64 TSAN_INTERCEPT(tmpfile64)
#else
#define TSAN_MAYBE_INTERCEPT_TMPFILE64
#endif

TSAN_INTERCEPTOR(uptr, fread, void *ptr, uptr size, uptr nmemb, void *f) {
  // libc file streams can call user-supplied functions, see fopencookie.
  {
    SCOPED_TSAN_INTERCEPTOR(fread, ptr, size, nmemb, f);
    MemoryAccessRange(thr, pc, (uptr)ptr, size * nmemb, true);
  }
  return REAL(fread)(ptr, size, nmemb, f);
}

TSAN_INTERCEPTOR(uptr, fwrite, const void *p, uptr size, uptr nmemb, void *f) {
  // libc file streams can call user-supplied functions, see fopencookie.
  {
    SCOPED_TSAN_INTERCEPTOR(fwrite, p, size, nmemb, f);
    MemoryAccessRange(thr, pc, (uptr)p, size * nmemb, false);
  }
  return REAL(fwrite)(p, size, nmemb, f);
}

static void FlushStreams() {
  // Flushing all the streams here may freeze the process if a child thread is
  // performing file stream operations at the same time.
  REAL(fflush)(stdout);
  REAL(fflush)(stderr);
}

TSAN_INTERCEPTOR(void, abort, int fake) {
  SCOPED_TSAN_INTERCEPTOR(abort, fake);
  FlushStreams();
  REAL(abort)(fake);
}

TSAN_INTERCEPTOR(int, puts, const char *s) {
  SCOPED_TSAN_INTERCEPTOR(puts, s);
  MemoryAccessRange(thr, pc, (uptr)s, internal_strlen(s), false);
  return REAL(puts)(s);
}

TSAN_INTERCEPTOR(int, rmdir, char *path) {
  SCOPED_TSAN_INTERCEPTOR(rmdir, path);
  Release(thr, pc, Dir2addr(path));
  int res = REAL(rmdir)(path);
  return res;
}

TSAN_INTERCEPTOR(int, closedir, void *dirp) {
  SCOPED_TSAN_INTERCEPTOR(closedir, dirp);
  int fd = dirfd(dirp);
  FdClose(thr, pc, fd);
  return REAL(closedir)(dirp);
}

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, epoll_ctl, int epfd, int op, int fd, void *ev) {
  SCOPED_TSAN_INTERCEPTOR(epoll_ctl, epfd, op, fd, ev);
  if (epfd >= 0)
    FdAccess(thr, pc, epfd);
  if (epfd >= 0 && fd >= 0)
    FdAccess(thr, pc, fd);
  if (op == EPOLL_CTL_ADD && epfd >= 0)
    FdRelease(thr, pc, epfd);
  int res = REAL(epoll_ctl)(epfd, op, fd, ev);
  return res;
}
#define TSAN_MAYBE_INTERCEPT_EPOLL_CTL TSAN_INTERCEPT(epoll_ctl)
#else
#define TSAN_MAYBE_INTERCEPT_EPOLL_CTL
#endif

#if SANITIZER_LINUX
TSAN_INTERCEPTOR(int, epoll_wait, int epfd, void *ev, int cnt, int timeout) {
  SCOPED_TSAN_INTERCEPTOR(epoll_wait, epfd, ev, cnt, timeout);
  if (epfd >= 0)
    FdAccess(thr, pc, epfd);
  int res = BLOCK_REAL(epoll_wait)(epfd, ev, cnt, timeout);
  if (res > 0 && epfd >= 0)
    FdAcquire(thr, pc, epfd);
  return res;
}
#define TSAN_MAYBE_INTERCEPT_EPOLL_WAIT TSAN_INTERCEPT(epoll_wait)
#else
#define TSAN_MAYBE_INTERCEPT_EPOLL_WAIT
#endif

namespace __tsan {

static void CallUserSignalHandler(ThreadState *thr, bool sync, bool acquire,
    bool sigact, int sig, my_siginfo_t *info, void *uctx) {
  if (acquire)
    Acquire(thr, 0, (uptr)&sigactions[sig]);
  // Ensure that the handler does not spoil errno.
  const int saved_errno = errno;
  errno = 99;
  // This code races with sigaction. Be careful to not read sa_sigaction twice.
  // Also need to remember pc for reporting before the call,
  // because the handler can reset it.
  volatile uptr pc = sigact ?
     (uptr)sigactions[sig].sa_sigaction :
     (uptr)sigactions[sig].sa_handler;
  if (pc != (uptr)SIG_DFL && pc != (uptr)SIG_IGN) {
    if (sigact)
      ((sigactionhandler_t)pc)(sig, info, uctx);
    else
      ((sighandler_t)pc)(sig);
  }
  // We do not detect errno spoiling for SIGTERM,
  // because some SIGTERM handlers do spoil errno but reraise SIGTERM,
  // tsan reports false positive in such case.
  // It's difficult to properly detect this situation (reraise),
  // because in async signal processing case (when handler is called directly
  // from rtl_generic_sighandler) we have not yet received the reraised
  // signal; and it looks too fragile to intercept all ways to reraise a signal.
  if (flags()->report_bugs && !sync && sig != SIGTERM && errno != 99) {
    VarSizeStackTrace stack;
    // StackTrace::GetNestInstructionPc(pc) is used because return address is
    // expected, OutputReport() will undo this.
    ObtainCurrentStack(thr, StackTrace::GetNextInstructionPc(pc), &stack);
    ThreadRegistryLock l(ctx->thread_registry);
    ScopedReport rep(ReportTypeErrnoInSignal);
    if (!IsFiredSuppression(ctx, ReportTypeErrnoInSignal, stack)) {
      rep.AddStack(stack, true);
      OutputReport(thr, rep);
    }
  }
  errno = saved_errno;
}

void ProcessPendingSignals(ThreadState *thr) {
  ThreadSignalContext *sctx = SigCtx(thr);
  if (sctx == 0 ||
      atomic_load(&sctx->have_pending_signals, memory_order_relaxed) == 0)
    return;
  atomic_store(&sctx->have_pending_signals, 0, memory_order_relaxed);
  atomic_fetch_add(&thr->in_signal_handler, 1, memory_order_relaxed);
  // These are too big for stack.
  static THREADLOCAL __sanitizer_sigset_t emptyset, oldset;
  CHECK_EQ(0, REAL(sigfillset)(&emptyset));
  CHECK_EQ(0, pthread_sigmask(SIG_SETMASK, &emptyset, &oldset));
  for (int sig = 0; sig < kSigCount; sig++) {
    SignalDesc *signal = &sctx->pending_signals[sig];
    if (signal->armed) {
      signal->armed = false;
      CallUserSignalHandler(thr, false, true, signal->sigaction, sig,
          &signal->siginfo, &signal->ctx);
    }
  }
  CHECK_EQ(0, pthread_sigmask(SIG_SETMASK, &oldset, 0));
  atomic_fetch_add(&thr->in_signal_handler, -1, memory_order_relaxed);
}

}  // namespace __tsan

static bool is_sync_signal(ThreadSignalContext *sctx, int sig) {
  return sig == SIGSEGV || sig == SIGBUS || sig == SIGILL ||
      sig == SIGABRT || sig == SIGFPE || sig == SIGPIPE || sig == SIGSYS ||
      // If we are sending signal to ourselves, we must process it now.
      (sctx && sig == sctx->int_signal_send);
}

void ALWAYS_INLINE rtl_generic_sighandler(bool sigact, int sig,
    my_siginfo_t *info, void *ctx) {
  ThreadState *thr = cur_thread();
  ThreadSignalContext *sctx = SigCtx(thr);
  if (sig < 0 || sig >= kSigCount) {
    VPrintf(1, "ThreadSanitizer: ignoring signal %d\n", sig);
    return;
  }
  // Don't mess with synchronous signals.
  const bool sync = is_sync_signal(sctx, sig);
  if (sync ||
      // If we are in blocking function, we can safely process it now
      // (but check if we are in a recursive interceptor,
      // i.e. pthread_join()->munmap()).
      (sctx && atomic_load(&sctx->in_blocking_func, memory_order_relaxed))) {
    atomic_fetch_add(&thr->in_signal_handler, 1, memory_order_relaxed);
    if (sctx && atomic_load(&sctx->in_blocking_func, memory_order_relaxed)) {
      // We ignore interceptors in blocking functions,
      // temporary enbled them again while we are calling user function.
      int const i = thr->ignore_interceptors;
      thr->ignore_interceptors = 0;
      atomic_store(&sctx->in_blocking_func, 0, memory_order_relaxed);
      CallUserSignalHandler(thr, sync, true, sigact, sig, info, ctx);
      thr->ignore_interceptors = i;
      atomic_store(&sctx->in_blocking_func, 1, memory_order_relaxed);
    } else {
      // Be very conservative with when we do acquire in this case.
      // It's unsafe to do acquire in async handlers, because ThreadState
      // can be in inconsistent state.
      // SIGSYS looks relatively safe -- it's synchronous and can actually
      // need some global state.
      bool acq = (sig == SIGSYS);
      CallUserSignalHandler(thr, sync, acq, sigact, sig, info, ctx);
    }
    atomic_fetch_add(&thr->in_signal_handler, -1, memory_order_relaxed);
    return;
  }

  if (sctx == 0)
    return;
  SignalDesc *signal = &sctx->pending_signals[sig];
  if (signal->armed == false) {
    signal->armed = true;
    signal->sigaction = sigact;
    if (info)
      internal_memcpy(&signal->siginfo, info, sizeof(*info));
    if (ctx)
      internal_memcpy(&signal->ctx, ctx, sizeof(signal->ctx));
    atomic_store(&sctx->have_pending_signals, 1, memory_order_relaxed);
  }
}

static void rtl_sighandler(int sig) {
  rtl_generic_sighandler(false, sig, 0, 0);
}

static void rtl_sigaction(int sig, my_siginfo_t *info, void *ctx) {
  rtl_generic_sighandler(true, sig, info, ctx);
}

TSAN_INTERCEPTOR(int, sigaction, int sig, sigaction_t *act, sigaction_t *old) {
  SCOPED_TSAN_INTERCEPTOR(sigaction, sig, act, old);
  if (old)
    internal_memcpy(old, &sigactions[sig], sizeof(*old));
  if (act == 0)
    return 0;
  // Copy act into sigactions[sig].
  // Can't use struct copy, because compiler can emit call to memcpy.
  // Can't use internal_memcpy, because it copies byte-by-byte,
  // and signal handler reads the sa_handler concurrently. It it can read
  // some bytes from old value and some bytes from new value.
  // Use volatile to prevent insertion of memcpy.
  sigactions[sig].sa_handler = *(volatile sighandler_t*)&act->sa_handler;
  sigactions[sig].sa_flags = *(volatile int*)&act->sa_flags;
  internal_memcpy(&sigactions[sig].sa_mask, &act->sa_mask,
      sizeof(sigactions[sig].sa_mask));
#if !SANITIZER_FREEBSD
  sigactions[sig].sa_restorer = act->sa_restorer;
#endif
  sigaction_t newact;
  internal_memcpy(&newact, act, sizeof(newact));
  REAL(sigfillset)(&newact.sa_mask);
  if (act->sa_handler != SIG_IGN && act->sa_handler != SIG_DFL) {
    if (newact.sa_flags & SA_SIGINFO)
      newact.sa_sigaction = rtl_sigaction;
    else
      newact.sa_handler = rtl_sighandler;
  }
  ReleaseStore(thr, pc, (uptr)&sigactions[sig]);
  int res = REAL(sigaction)(sig, &newact, 0);
  return res;
}

TSAN_INTERCEPTOR(sighandler_t, signal, int sig, sighandler_t h) {
  sigaction_t act;
  act.sa_handler = h;
  REAL(memset)(&act.sa_mask, -1, sizeof(act.sa_mask));
  act.sa_flags = 0;
  sigaction_t old;
  int res = sigaction(sig, &act, &old);
  if (res)
    return SIG_ERR;
  return old.sa_handler;
}

TSAN_INTERCEPTOR(int, sigsuspend, const __sanitizer_sigset_t *mask) {
  SCOPED_TSAN_INTERCEPTOR(sigsuspend, mask);
  return REAL(sigsuspend)(mask);
}

TSAN_INTERCEPTOR(int, raise, int sig) {
  SCOPED_TSAN_INTERCEPTOR(raise, sig);
  ThreadSignalContext *sctx = SigCtx(thr);
  CHECK_NE(sctx, 0);
  int prev = sctx->int_signal_send;
  sctx->int_signal_send = sig;
  int res = REAL(raise)(sig);
  CHECK_EQ(sctx->int_signal_send, sig);
  sctx->int_signal_send = prev;
  return res;
}

TSAN_INTERCEPTOR(int, kill, int pid, int sig) {
  SCOPED_TSAN_INTERCEPTOR(kill, pid, sig);
  ThreadSignalContext *sctx = SigCtx(thr);
  CHECK_NE(sctx, 0);
  int prev = sctx->int_signal_send;
  if (pid == (int)internal_getpid()) {
    sctx->int_signal_send = sig;
  }
  int res = REAL(kill)(pid, sig);
  if (pid == (int)internal_getpid()) {
    CHECK_EQ(sctx->int_signal_send, sig);
    sctx->int_signal_send = prev;
  }
  return res;
}

TSAN_INTERCEPTOR(int, pthread_kill, void *tid, int sig) {
  SCOPED_TSAN_INTERCEPTOR(pthread_kill, tid, sig);
  ThreadSignalContext *sctx = SigCtx(thr);
  CHECK_NE(sctx, 0);
  int prev = sctx->int_signal_send;
  if (tid == pthread_self()) {
    sctx->int_signal_send = sig;
  }
  int res = REAL(pthread_kill)(tid, sig);
  if (tid == pthread_self()) {
    CHECK_EQ(sctx->int_signal_send, sig);
    sctx->int_signal_send = prev;
  }
  return res;
}

TSAN_INTERCEPTOR(int, gettimeofday, void *tv, void *tz) {
  SCOPED_TSAN_INTERCEPTOR(gettimeofday, tv, tz);
  // It's intercepted merely to process pending signals.
  return REAL(gettimeofday)(tv, tz);
}

TSAN_INTERCEPTOR(int, getaddrinfo, void *node, void *service,
    void *hints, void *rv) {
  SCOPED_TSAN_INTERCEPTOR(getaddrinfo, node, service, hints, rv);
  // We miss atomic synchronization in getaddrinfo,
  // and can report false race between malloc and free
  // inside of getaddrinfo. So ignore memory accesses.
  ThreadIgnoreBegin(thr, pc);
  int res = REAL(getaddrinfo)(node, service, hints, rv);
  ThreadIgnoreEnd(thr, pc);
  return res;
}

TSAN_INTERCEPTOR(int, fork, int fake) {
  if (cur_thread()->in_symbolizer)
    return REAL(fork)(fake);
  SCOPED_INTERCEPTOR_RAW(fork, fake);
  ForkBefore(thr, pc);
  int pid = REAL(fork)(fake);
  if (pid == 0) {
    // child
    ForkChildAfter(thr, pc);
    FdOnFork(thr, pc);
  } else if (pid > 0) {
    // parent
    ForkParentAfter(thr, pc);
  } else {
    // error
    ForkParentAfter(thr, pc);
  }
  return pid;
}

TSAN_INTERCEPTOR(int, vfork, int fake) {
  // Some programs (e.g. openjdk) call close for all file descriptors
  // in the child process. Under tsan it leads to false positives, because
  // address space is shared, so the parent process also thinks that
  // the descriptors are closed (while they are actually not).
  // This leads to false positives due to missed synchronization.
  // Strictly saying this is undefined behavior, because vfork child is not
  // allowed to call any functions other than exec/exit. But this is what
  // openjdk does, so we want to handle it.
  // We could disable interceptors in the child process. But it's not possible
  // to simply intercept and wrap vfork, because vfork child is not allowed
  // to return from the function that calls vfork, and that's exactly what
  // we would do. So this would require some assembly trickery as well.
  // Instead we simply turn vfork into fork.
  return WRAP(fork)(fake);
}

#if !SANITIZER_MAC
typedef int (*dl_iterate_phdr_cb_t)(__sanitizer_dl_phdr_info *info, SIZE_T size,
                                    void *data);
struct dl_iterate_phdr_data {
  ThreadState *thr;
  uptr pc;
  dl_iterate_phdr_cb_t cb;
  void *data;
};

static bool IsAppNotRodata(uptr addr) {
  return IsAppMem(addr) && *(u64*)MemToShadow(addr) != kShadowRodata;
}

static int dl_iterate_phdr_cb(__sanitizer_dl_phdr_info *info, SIZE_T size,
                              void *data) {
  dl_iterate_phdr_data *cbdata = (dl_iterate_phdr_data *)data;
  // dlopen/dlclose allocate/free dynamic-linker-internal memory, which is later
  // accessible in dl_iterate_phdr callback. But we don't see synchronization
  // inside of dynamic linker, so we "unpoison" it here in order to not
  // produce false reports. Ignoring malloc/free in dlopen/dlclose is not enough
  // because some libc functions call __libc_dlopen.
  if (info && IsAppNotRodata((uptr)info->dlpi_name))
    MemoryResetRange(cbdata->thr, cbdata->pc, (uptr)info->dlpi_name,
                     internal_strlen(info->dlpi_name));
  int res = cbdata->cb(info, size, cbdata->data);
  // Perform the check one more time in case info->dlpi_name was overwritten
  // by user callback.
  if (info && IsAppNotRodata((uptr)info->dlpi_name))
    MemoryResetRange(cbdata->thr, cbdata->pc, (uptr)info->dlpi_name,
                     internal_strlen(info->dlpi_name));
  return res;
}

TSAN_INTERCEPTOR(int, dl_iterate_phdr, dl_iterate_phdr_cb_t cb, void *data) {
  SCOPED_TSAN_INTERCEPTOR(dl_iterate_phdr, cb, data);
  dl_iterate_phdr_data cbdata;
  cbdata.thr = thr;
  cbdata.pc = pc;
  cbdata.cb = cb;
  cbdata.data = data;
  int res = REAL(dl_iterate_phdr)(dl_iterate_phdr_cb, &cbdata);
  return res;
}
#endif

static int OnExit(ThreadState *thr) {
  int status = Finalize(thr);
  FlushStreams();
  return status;
}

struct TsanInterceptorContext {
  ThreadState *thr;
  const uptr caller_pc;
  const uptr pc;
};

#if !SANITIZER_MAC
static void HandleRecvmsg(ThreadState *thr, uptr pc,
    __sanitizer_msghdr *msg) {
  int fds[64];
  int cnt = ExtractRecvmsgFDs(msg, fds, ARRAY_SIZE(fds));
  for (int i = 0; i < cnt; i++)
    FdEventCreate(thr, pc, fds[i]);
}
#endif

#include "sanitizer_common/sanitizer_platform_interceptors.h"
// Causes interceptor recursion (getaddrinfo() and fopen())
#undef SANITIZER_INTERCEPT_GETADDRINFO
// There interceptors do not seem to be strictly necessary for tsan.
// But we see cases where the interceptors consume 70% of execution time.
// Memory blocks passed to fgetgrent_r are "written to" by tsan several times.
// First, there is some recursion (getgrnam_r calls fgetgrent_r), and each
// function "writes to" the buffer. Then, the same memory is "written to"
// twice, first as buf and then as pwbufp (both of them refer to the same
// addresses).
#undef SANITIZER_INTERCEPT_GETPWENT
#undef SANITIZER_INTERCEPT_GETPWENT_R
#undef SANITIZER_INTERCEPT_FGETPWENT
#undef SANITIZER_INTERCEPT_GETPWNAM_AND_FRIENDS
#undef SANITIZER_INTERCEPT_GETPWNAM_R_AND_FRIENDS
// __tls_get_addr can be called with mis-aligned stack due to:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58066
// There are two potential issues:
// 1. Sanitizer code contains a MOVDQA spill (it does not seem to be the case
// right now). or 2. ProcessPendingSignal calls user handler which contains
// MOVDQA spill (this happens right now).
// Since the interceptor only initializes memory for msan, the simplest solution
// is to disable the interceptor in tsan (other sanitizers do not call
// signal handlers from COMMON_INTERCEPTOR_ENTER).
// As __tls_get_addr has been intercepted in the past, to avoid breaking
// libtsan ABI, keep it around, but just call the real function.
#if SANITIZER_INTERCEPT_TLS_GET_ADDR
#define NEED_TLS_GET_ADDR
#endif
#undef SANITIZER_INTERCEPT_TLS_GET_ADDR

#define COMMON_INTERCEPT_FUNCTION(name) INTERCEPT_FUNCTION(name)

#define COMMON_INTERCEPTOR_WRITE_RANGE(ctx, ptr, size)                    \
  MemoryAccessRange(((TsanInterceptorContext *)ctx)->thr,                 \
                    ((TsanInterceptorContext *)ctx)->pc, (uptr)ptr, size, \
                    true)

#define COMMON_INTERCEPTOR_READ_RANGE(ctx, ptr, size)                       \
  MemoryAccessRange(((TsanInterceptorContext *) ctx)->thr,                  \
                    ((TsanInterceptorContext *) ctx)->pc, (uptr) ptr, size, \
                    false)

#define COMMON_INTERCEPTOR_ENTER(ctx, func, ...)      \
  SCOPED_TSAN_INTERCEPTOR(func, __VA_ARGS__);         \
  TsanInterceptorContext _ctx = {thr, caller_pc, pc}; \
  ctx = (void *)&_ctx;                                \
  (void) ctx;

#define COMMON_INTERCEPTOR_ENTER_NOIGNORE(ctx, func, ...) \
  SCOPED_INTERCEPTOR_RAW(func, __VA_ARGS__);              \
  TsanInterceptorContext _ctx = {thr, caller_pc, pc};     \
  ctx = (void *)&_ctx;                                    \
  (void) ctx;

#define COMMON_INTERCEPTOR_FILE_OPEN(ctx, file, path) \
  Acquire(thr, pc, File2addr(path));                  \
  if (file) {                                         \
    int fd = fileno_unlocked(file);                   \
    if (fd >= 0) FdFileCreate(thr, pc, fd);           \
  }

#define COMMON_INTERCEPTOR_FILE_CLOSE(ctx, file) \
  if (file) {                                    \
    int fd = fileno_unlocked(file);              \
    if (fd >= 0) FdClose(thr, pc, fd);           \
  }

#define COMMON_INTERCEPTOR_LIBRARY_LOADED(filename, handle) \
  libignore()->OnLibraryLoaded(filename)

#define COMMON_INTERCEPTOR_LIBRARY_UNLOADED() \
  libignore()->OnLibraryUnloaded()

#define COMMON_INTERCEPTOR_ACQUIRE(ctx, u) \
  Acquire(((TsanInterceptorContext *) ctx)->thr, pc, u)

#define COMMON_INTERCEPTOR_RELEASE(ctx, u) \
  Release(((TsanInterceptorContext *) ctx)->thr, pc, u)

#define COMMON_INTERCEPTOR_DIR_ACQUIRE(ctx, path) \
  Acquire(((TsanInterceptorContext *) ctx)->thr, pc, Dir2addr(path))

#define COMMON_INTERCEPTOR_FD_ACQUIRE(ctx, fd) \
  FdAcquire(((TsanInterceptorContext *) ctx)->thr, pc, fd)

#define COMMON_INTERCEPTOR_FD_RELEASE(ctx, fd) \
  FdRelease(((TsanInterceptorContext *) ctx)->thr, pc, fd)

#define COMMON_INTERCEPTOR_FD_ACCESS(ctx, fd) \
  FdAccess(((TsanInterceptorContext *) ctx)->thr, pc, fd)

#define COMMON_INTERCEPTOR_FD_SOCKET_ACCEPT(ctx, fd, newfd) \
  FdSocketAccept(((TsanInterceptorContext *) ctx)->thr, pc, fd, newfd)

#define COMMON_INTERCEPTOR_SET_THREAD_NAME(ctx, name) \
  ThreadSetName(((TsanInterceptorContext *) ctx)->thr, name)

#define COMMON_INTERCEPTOR_SET_PTHREAD_NAME(ctx, thread, name) \
  __tsan::ctx->thread_registry->SetThreadNameByUserId(thread, name)

#define COMMON_INTERCEPTOR_BLOCK_REAL(name) BLOCK_REAL(name)

#define COMMON_INTERCEPTOR_ON_EXIT(ctx) \
  OnExit(((TsanInterceptorContext *) ctx)->thr)

#define COMMON_INTERCEPTOR_MUTEX_LOCK(ctx, m) \
  MutexLock(((TsanInterceptorContext *)ctx)->thr, \
            ((TsanInterceptorContext *)ctx)->pc, (uptr)m)

#define COMMON_INTERCEPTOR_MUTEX_UNLOCK(ctx, m) \
  MutexUnlock(((TsanInterceptorContext *)ctx)->thr, \
            ((TsanInterceptorContext *)ctx)->pc, (uptr)m)

#define COMMON_INTERCEPTOR_MUTEX_REPAIR(ctx, m) \
  MutexRepair(((TsanInterceptorContext *)ctx)->thr, \
            ((TsanInterceptorContext *)ctx)->pc, (uptr)m)

#if !SANITIZER_MAC
#define COMMON_INTERCEPTOR_HANDLE_RECVMSG(ctx, msg) \
  HandleRecvmsg(((TsanInterceptorContext *)ctx)->thr, \
      ((TsanInterceptorContext *)ctx)->pc, msg)
#endif

#define COMMON_INTERCEPTOR_GET_TLS_RANGE(begin, end)                           \
  if (TsanThread *t = GetCurrentThread()) {                                    \
    *begin = t->tls_begin();                                                   \
    *end = t->tls_end();                                                       \
  } else {                                                                     \
    *begin = *end = 0;                                                         \
  }

#include "sanitizer_common/sanitizer_common_interceptors.inc"

#define TSAN_SYSCALL() \
  ThreadState *thr = cur_thread(); \
  if (thr->ignore_interceptors) \
    return; \
  ScopedSyscall scoped_syscall(thr) \
/**/

struct ScopedSyscall {
  ThreadState *thr;

  explicit ScopedSyscall(ThreadState *thr)
      : thr(thr) {
    Initialize(thr);
  }

  ~ScopedSyscall() {
    ProcessPendingSignals(thr);
  }
};

#if !SANITIZER_NETBSD && !SANITIZER_MAC
static void syscall_access_range(uptr pc, uptr p, uptr s, bool write) {
  TSAN_SYSCALL();
  MemoryAccessRange(thr, pc, p, s, write);
}

static void syscall_acquire(uptr pc, uptr addr) {
  TSAN_SYSCALL();
  Acquire(thr, pc, addr);
  DPrintf("syscall_acquire(%p)\n", addr);
}

static void syscall_release(uptr pc, uptr addr) {
  TSAN_SYSCALL();
  DPrintf("syscall_release(%p)\n", addr);
  Release(thr, pc, addr);
}

static void syscall_fd_close(uptr pc, int fd) {
  TSAN_SYSCALL();
  FdClose(thr, pc, fd);
}
#endif

static USED void syscall_fd_acquire(uptr pc, int fd) {
  TSAN_SYSCALL();
  FdAcquire(thr, pc, fd);
  DPrintf("syscall_fd_acquire(%p)\n", fd);
}

static USED void syscall_fd_release(uptr pc, int fd) {
  TSAN_SYSCALL();
  DPrintf("syscall_fd_release(%p)\n", fd);
  FdRelease(thr, pc, fd);
}

#if !SANITIZER_NETBSD
static void syscall_pre_fork(uptr pc) {
  TSAN_SYSCALL();
  ForkBefore(thr, pc);
}

static void syscall_post_fork(uptr pc, int pid) {
  TSAN_SYSCALL();
  if (pid == 0) {
    // child
    ForkChildAfter(thr, pc);
    FdOnFork(thr, pc);
  } else if (pid > 0) {
    // parent
    ForkParentAfter(thr, pc);
  } else {
    // error
    ForkParentAfter(thr, pc);
  }
}
#endif

#define COMMON_SYSCALL_PRE_READ_RANGE(p, s) \
  syscall_access_range(GET_CALLER_PC(), (uptr)(p), (uptr)(s), false)

#define COMMON_SYSCALL_PRE_WRITE_RANGE(p, s) \
  syscall_access_range(GET_CALLER_PC(), (uptr)(p), (uptr)(s), true)

#define COMMON_SYSCALL_POST_READ_RANGE(p, s) \
  do {                                       \
    (void)(p);                               \
    (void)(s);                               \
  } while (false)

#define COMMON_SYSCALL_POST_WRITE_RANGE(p, s) \
  do {                                        \
    (void)(p);                                \
    (void)(s);                                \
  } while (false)

#define COMMON_SYSCALL_ACQUIRE(addr) \
    syscall_acquire(GET_CALLER_PC(), (uptr)(addr))

#define COMMON_SYSCALL_RELEASE(addr) \
    syscall_release(GET_CALLER_PC(), (uptr)(addr))

#define COMMON_SYSCALL_FD_CLOSE(fd) syscall_fd_close(GET_CALLER_PC(), fd)

#define COMMON_SYSCALL_FD_ACQUIRE(fd) syscall_fd_acquire(GET_CALLER_PC(), fd)

#define COMMON_SYSCALL_FD_RELEASE(fd) syscall_fd_release(GET_CALLER_PC(), fd)

#define COMMON_SYSCALL_PRE_FORK() \
  syscall_pre_fork(GET_CALLER_PC())

#define COMMON_SYSCALL_POST_FORK(res) \
  syscall_post_fork(GET_CALLER_PC(), res)

#include "sanitizer_common/sanitizer_common_syscalls.inc"

#ifdef NEED_TLS_GET_ADDR
TSAN_INTERCEPTOR(void *, __tls_get_addr, void *arg) {
  return REAL(__tls_get_addr)(arg);
}
#endif

namespace __tsan {

static void finalize(void *arg) {
  ThreadState *thr = cur_thread();
  int status = Finalize(thr);
  // Make sure the output is not lost.
  FlushStreams();
  if (status)
    Die();
}

#if !SANITIZER_MAC
static void unreachable() {
  Report("FATAL: ThreadSanitizer: unreachable called\n");
  Die();
}
#endif

void InitializeInterceptors() {
#if !SANITIZER_MAC
  // We need to setup it early, because functions like dlsym() can call it.
  REAL(memset) = internal_memset;
  REAL(memcpy) = internal_memcpy;
#endif

  // Instruct libc malloc to consume less memory.
#if SANITIZER_LINUX
  mallopt(1, 0);  // M_MXFAST
  mallopt(-3, 32*1024);  // M_MMAP_THRESHOLD
#endif

  InitializeCommonInterceptors();

#if !SANITIZER_MAC
  // We can not use TSAN_INTERCEPT to get setjmp addr,
  // because it does &setjmp and setjmp is not present in some versions of libc.
  using __interception::GetRealFunctionAddress;
  GetRealFunctionAddress("setjmp", (uptr*)&REAL(setjmp), 0, 0);
  GetRealFunctionAddress("_setjmp", (uptr*)&REAL(_setjmp), 0, 0);
  GetRealFunctionAddress("sigsetjmp", (uptr*)&REAL(sigsetjmp), 0, 0);
  GetRealFunctionAddress("__sigsetjmp", (uptr*)&REAL(__sigsetjmp), 0, 0);
#endif

  TSAN_INTERCEPT(longjmp);
  TSAN_INTERCEPT(siglongjmp);

  TSAN_INTERCEPT(malloc);
  TSAN_INTERCEPT(__libc_memalign);
  TSAN_INTERCEPT(calloc);
  TSAN_INTERCEPT(realloc);
  TSAN_INTERCEPT(free);
  TSAN_INTERCEPT(cfree);
  TSAN_INTERCEPT(mmap);
  TSAN_MAYBE_INTERCEPT_MMAP64;
  TSAN_INTERCEPT(munmap);
  TSAN_MAYBE_INTERCEPT_MEMALIGN;
  TSAN_INTERCEPT(valloc);
  TSAN_MAYBE_INTERCEPT_PVALLOC;
  TSAN_INTERCEPT(posix_memalign);

  TSAN_INTERCEPT(strlen);
  TSAN_INTERCEPT(memset);
  TSAN_INTERCEPT(memcpy);
  TSAN_INTERCEPT(memmove);
  TSAN_INTERCEPT(strchr);
  TSAN_INTERCEPT(strchrnul);
  TSAN_INTERCEPT(strrchr);
  TSAN_INTERCEPT(strcpy);  // NOLINT
  TSAN_INTERCEPT(strncpy);
  TSAN_INTERCEPT(strdup);

  TSAN_INTERCEPT(pthread_create);
  TSAN_INTERCEPT(pthread_join);
  TSAN_INTERCEPT(pthread_detach);

  TSAN_INTERCEPT_VER(pthread_cond_init, PTHREAD_ABI_BASE);
  TSAN_INTERCEPT_VER(pthread_cond_signal, PTHREAD_ABI_BASE);
  TSAN_INTERCEPT_VER(pthread_cond_broadcast, PTHREAD_ABI_BASE);
  TSAN_INTERCEPT_VER(pthread_cond_wait, PTHREAD_ABI_BASE);
  TSAN_INTERCEPT_VER(pthread_cond_timedwait, PTHREAD_ABI_BASE);
  TSAN_INTERCEPT_VER(pthread_cond_destroy, PTHREAD_ABI_BASE);

  TSAN_INTERCEPT(pthread_mutex_init);
  TSAN_INTERCEPT(pthread_mutex_destroy);
  TSAN_INTERCEPT(pthread_mutex_trylock);
  TSAN_INTERCEPT(pthread_mutex_timedlock);

  TSAN_INTERCEPT(pthread_spin_init);
  TSAN_INTERCEPT(pthread_spin_destroy);
  TSAN_INTERCEPT(pthread_spin_lock);
  TSAN_INTERCEPT(pthread_spin_trylock);
  TSAN_INTERCEPT(pthread_spin_unlock);

  TSAN_INTERCEPT(pthread_rwlock_init);
  TSAN_INTERCEPT(pthread_rwlock_destroy);
  TSAN_INTERCEPT(pthread_rwlock_rdlock);
  TSAN_INTERCEPT(pthread_rwlock_tryrdlock);
  TSAN_INTERCEPT(pthread_rwlock_timedrdlock);
  TSAN_INTERCEPT(pthread_rwlock_wrlock);
  TSAN_INTERCEPT(pthread_rwlock_trywrlock);
  TSAN_INTERCEPT(pthread_rwlock_timedwrlock);
  TSAN_INTERCEPT(pthread_rwlock_unlock);

  TSAN_INTERCEPT(pthread_barrier_init);
  TSAN_INTERCEPT(pthread_barrier_destroy);
  TSAN_INTERCEPT(pthread_barrier_wait);

  TSAN_INTERCEPT(pthread_once);

  TSAN_INTERCEPT(stat);
  TSAN_MAYBE_INTERCEPT___XSTAT;
  TSAN_MAYBE_INTERCEPT_STAT64;
  TSAN_MAYBE_INTERCEPT___XSTAT64;
  TSAN_INTERCEPT(lstat);
  TSAN_MAYBE_INTERCEPT___LXSTAT;
  TSAN_MAYBE_INTERCEPT_LSTAT64;
  TSAN_MAYBE_INTERCEPT___LXSTAT64;
  TSAN_INTERCEPT(fstat);
  TSAN_MAYBE_INTERCEPT___FXSTAT;
  TSAN_MAYBE_INTERCEPT_FSTAT64;
  TSAN_MAYBE_INTERCEPT___FXSTAT64;
  TSAN_INTERCEPT(open);
  TSAN_MAYBE_INTERCEPT_OPEN64;
  TSAN_INTERCEPT(creat);
  TSAN_MAYBE_INTERCEPT_CREAT64;
  TSAN_INTERCEPT(dup);
  TSAN_INTERCEPT(dup2);
  TSAN_INTERCEPT(dup3);
  TSAN_MAYBE_INTERCEPT_EVENTFD;
  TSAN_MAYBE_INTERCEPT_SIGNALFD;
  TSAN_MAYBE_INTERCEPT_INOTIFY_INIT;
  TSAN_MAYBE_INTERCEPT_INOTIFY_INIT1;
  TSAN_INTERCEPT(socket);
  TSAN_INTERCEPT(socketpair);
  TSAN_INTERCEPT(connect);
  TSAN_INTERCEPT(bind);
  TSAN_INTERCEPT(listen);
  TSAN_MAYBE_INTERCEPT_EPOLL_CREATE;
  TSAN_MAYBE_INTERCEPT_EPOLL_CREATE1;
  TSAN_INTERCEPT(close);
  TSAN_MAYBE_INTERCEPT___CLOSE;
  TSAN_MAYBE_INTERCEPT___RES_ICLOSE;
  TSAN_INTERCEPT(pipe);
  TSAN_INTERCEPT(pipe2);

  TSAN_INTERCEPT(send);
  TSAN_INTERCEPT(sendmsg);
  TSAN_INTERCEPT(recv);

  TSAN_INTERCEPT(unlink);
  TSAN_INTERCEPT(tmpfile);
  TSAN_MAYBE_INTERCEPT_TMPFILE64;
  TSAN_INTERCEPT(fread);
  TSAN_INTERCEPT(fwrite);
  TSAN_INTERCEPT(abort);
  TSAN_INTERCEPT(puts);
  TSAN_INTERCEPT(rmdir);
  TSAN_INTERCEPT(closedir);

  TSAN_MAYBE_INTERCEPT_EPOLL_CTL;
  TSAN_MAYBE_INTERCEPT_EPOLL_WAIT;

  TSAN_INTERCEPT(sigaction);
  TSAN_INTERCEPT(signal);
  TSAN_INTERCEPT(sigsuspend);
  TSAN_INTERCEPT(raise);
  TSAN_INTERCEPT(kill);
  TSAN_INTERCEPT(pthread_kill);
  TSAN_INTERCEPT(sleep);
  TSAN_INTERCEPT(usleep);
  TSAN_INTERCEPT(nanosleep);
  TSAN_INTERCEPT(gettimeofday);
  TSAN_INTERCEPT(getaddrinfo);

  TSAN_INTERCEPT(fork);
  TSAN_INTERCEPT(vfork);
  TSAN_INTERCEPT(dl_iterate_phdr);
  TSAN_INTERCEPT(on_exit);
  TSAN_INTERCEPT(__cxa_atexit);
  TSAN_INTERCEPT(_exit);

#ifdef NEED_TLS_GET_ADDR
  TSAN_INTERCEPT(__tls_get_addr);
#endif

#if !SANITIZER_MAC
  // Need to setup it, because interceptors check that the function is resolved.
  // But atexit is emitted directly into the module, so can't be resolved.
  REAL(atexit) = (int(*)(void(*)()))unreachable;
#endif

  if (REAL(__cxa_atexit)(&finalize, 0, 0)) {
    Printf("ThreadSanitizer: failed to setup atexit callback\n");
    Die();
  }

  if (pthread_key_create(&g_thread_finalize_key, &thread_finalize)) {
    Printf("ThreadSanitizer: failed to create thread key\n");
    Die();
  }

  FdInit();
}

}  // namespace __tsan
