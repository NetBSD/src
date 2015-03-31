//===-- asan_linux.cc -----------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// Linux-specific details.
//===----------------------------------------------------------------------===//
#if defined(__linux__) || defined(__NetBSD__)

#include "asan_interceptors.h"
#include "asan_internal.h"
#include "asan_thread.h"
#include "asan_thread_registry.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_procmaps.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <unwind.h>

#if !ASAN_ANDROID
// FIXME: where to get ucontext on Android?
#include <sys/ucontext.h>
#endif

extern "C" void* _DYNAMIC;

namespace __asan {

void MaybeReexec() {
  // No need to re-exec on Linux.
}

void *AsanDoesNotSupportStaticLinkage() {
  // This will fail to link with -static.
  return &_DYNAMIC;  // defined in link.h
}

void GetPcSpBp(void *context, uptr *pc, uptr *sp, uptr *bp) {
#ifdef __NetBSD__
# define __UC_MACHINE_FP(ucontext, r) \
    (ucontext)->uc_mcontext.__gregs[(r)]
/*
 * Unfortunately we don't have a portable frame pointer (yet)
 */
# if defined(__alpha__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_S6)
# elif defined(__arm__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_FP)
# elif defined(__x86_64__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_RBP)
# elif defined(__i386__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_EBP)
# elif defined(__m68k__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_A6)
# elif defined(__mips__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_S8)
# elif defined(__powerpc__) || defined(__powerpc64__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_R1)
# elif defined(__riscv__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_S0)
# elif defined(__sparc__)
#  define _UC_MACHINE_FP(ucontext) sp[15]
# elif defined(__sh3__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_R14)
# elif defined(__vax__)
#  define _UC_MACHINE_FP(ucontext) __UC_MACHINE_FP(ucontext, _REG_FP)
# else
#  define _UC_MACHINE_FP(ucontext) 0
# endif
  ucontext_t *ucontext = (ucontext_t*)context;
  *pc = _UC_MACHINE_PC(ucontext);
  *sp = _UC_MACHINE_SP(ucontext);
  *bp = _UC_MACHINE_FP(ucontext);
#elif ASAN_ANDROID
  *pc = *sp = *bp = 0;
#elif defined(__arm__)
  ucontext_t *ucontext = (ucontext_t*)context;
  *pc = ucontext->uc_mcontext.arm_pc;
  *bp = ucontext->uc_mcontext.arm_fp;
  *sp = ucontext->uc_mcontext.arm_sp;
# elif defined(__x86_64__)
  ucontext_t *ucontext = (ucontext_t*)context;
  *pc = ucontext->uc_mcontext.gregs[REG_RIP];
  *bp = ucontext->uc_mcontext.gregs[REG_RBP];
  *sp = ucontext->uc_mcontext.gregs[REG_RSP];
# elif defined(__i386__)
  ucontext_t *ucontext = (ucontext_t*)context;
  *pc = ucontext->uc_mcontext.gregs[REG_EIP];
  *bp = ucontext->uc_mcontext.gregs[REG_EBP];
  *sp = ucontext->uc_mcontext.gregs[REG_ESP];
# elif defined(__powerpc__) || defined(__powerpc64__)
  ucontext_t *ucontext = (ucontext_t*)context;
  *pc = ucontext->uc_mcontext.regs->nip;
  *sp = ucontext->uc_mcontext.regs->gpr[PT_R1];
  // The powerpc{,64}-linux ABIs do not specify r31 as the frame
  // pointer, but GCC always uses r31 when we need a frame pointer.
  *bp = ucontext->uc_mcontext.regs->gpr[PT_R31];
# elif defined(__riscv__)
  ucontext_t *ucontext = (ucontext_t*)context;
  *pc = ucontext->uc_mcontext.gregs[REG_PC];
  *bp = ucontext->uc_mcontext.gregs[REG_S0];
  *sp = ucontext->uc_mcontext.gregs[REG_SP];
# elif defined(__sparc__)
  ucontext_t *ucontext = (ucontext_t*)context;
  uptr *stk_ptr;
# if defined (__arch64__)
  *pc = ucontext->uc_mcontext.mc_gregs[MC_PC];
  *sp = ucontext->uc_mcontext.mc_gregs[MC_O6];
  stk_ptr = (uptr *) (*sp + 2047);
  *bp = stk_ptr[15];
# else
  *pc = ucontext->uc_mcontext.gregs[REG_PC];
  *sp = ucontext->uc_mcontext.gregs[REG_O6];
  stk_ptr = (uptr *) *sp;
  *bp = stk_ptr[15];
# endif
#else
# error "Unsupported arch"
#endif
}

bool AsanInterceptsSignal(int signum) {
  return signum == SIGSEGV && flags()->handle_segv;
}

void AsanPlatformThreadInit() {
  // Nothing here for now.
}

void GetStackTrace(StackTrace *stack, uptr max_s, uptr pc, uptr bp, bool fast) {
#if defined(__arm__) || \
    defined(__powerpc__) || defined(__powerpc64__) || \
    defined(__sparc__)
  fast = false;
#endif
  if (!fast)
    return stack->SlowUnwindStack(pc, max_s);
  stack->size = 0;
  stack->trace[0] = pc;
  if (max_s > 1) {
    stack->max_size = max_s;
    if (!asan_inited) return;
    if (AsanThread *t = asanThreadRegistry().GetCurrent())
      stack->FastUnwindStack(pc, bp, t->stack_top(), t->stack_bottom());
  }
}

#if !ASAN_ANDROID
void ReadContextStack(void *context, uptr *stack, uptr *ssize) {
  ucontext_t *ucp = (ucontext_t*)context;
  *stack = (uptr)ucp->uc_stack.ss_sp;
  *ssize = ucp->uc_stack.ss_size;
}
#else
void ReadContextStack(void *context, uptr *stack, uptr *ssize) {
  UNIMPLEMENTED();
}
#endif

}  // namespace __asan

#endif  // __linux__ || __NetBSD__
