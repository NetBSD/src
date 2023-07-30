//===-- sanitizer_stacktrace.h ----------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is shared between AddressSanitizer and ThreadSanitizer
// run-time libraries.
//===----------------------------------------------------------------------===//
#ifndef SANITIZER_STACKTRACE_H
#define SANITIZER_STACKTRACE_H

#include "sanitizer_common.h"
#include "sanitizer_internal_defs.h"
#include "sanitizer_platform.h"

namespace __sanitizer {

static const u32 kStackTraceMax = 256;

#if SANITIZER_LINUX && defined(__mips__)
# define SANITIZER_CAN_FAST_UNWIND 0
#elif SANITIZER_WINDOWS
# define SANITIZER_CAN_FAST_UNWIND 0
#else
# define SANITIZER_CAN_FAST_UNWIND 1
#endif

// Fast unwind is the only option on Mac for now; we will need to
// revisit this macro when slow unwind works on Mac, see
// https://github.com/google/sanitizers/issues/137
#if SANITIZER_MAC
#  define SANITIZER_CAN_SLOW_UNWIND 0
#else
# define SANITIZER_CAN_SLOW_UNWIND 1
#endif

struct StackTrace {
  const uptr *trace;
  u32 size;
  u32 tag;

  static const int TAG_UNKNOWN = 0;
  static const int TAG_ALLOC = 1;
  static const int TAG_DEALLOC = 2;
  static const int TAG_CUSTOM = 100; // Tool specific tags start here.

  StackTrace() : trace(nullptr), size(0), tag(0) {}
  StackTrace(const uptr *trace, u32 size) : trace(trace), size(size), tag(0) {}
  StackTrace(const uptr *trace, u32 size, u32 tag)
      : trace(trace), size(size), tag(tag) {}

  // Prints a symbolized stacktrace, followed by an empty line.
  void Print() const;

  // Prints a symbolized stacktrace to the output string, followed by an empty
  // line.
  void PrintTo(InternalScopedString *output) const;

  // Prints a symbolized stacktrace to the output buffer, followed by an empty
  // line. Returns the number of symbols that should have been written to buffer
  // (not including trailing '\0'). Thus, the string is truncated iff return
  // value is not less than "out_buf_size".
  uptr PrintTo(char *out_buf, uptr out_buf_size) const;

  static bool WillUseFastUnwind(bool request_fast_unwind) {
    if (!SANITIZER_CAN_FAST_UNWIND)
      return false;
    else if (!SANITIZER_CAN_SLOW_UNWIND)
      return true;
    return request_fast_unwind;
  }

  static uptr GetCurrentPc();
  static inline uptr GetPreviousInstructionPc(uptr pc);
  static uptr GetNextInstructionPc(uptr pc);
};

// Performance-critical, must be in the header.
ALWAYS_INLINE
uptr StackTrace::GetPreviousInstructionPc(uptr pc) {
#if defined(__arm__)
  // T32 (Thumb) branch instructions might be 16 or 32 bit long,
  // so we return (pc-2) in that case in order to be safe.
  // For A32 mode we return (pc-4) because all instructions are 32 bit long.
  return (pc - 3) & (~1);
#elif defined(__powerpc__) || defined(__powerpc64__) || defined(__aarch64__)
  // PCs are always 4 byte aligned.
  return pc - 4;
#elif defined(__sparc__) || defined(__mips__)
  return pc - 8;
#elif SANITIZER_RISCV64
  // RV-64 has variable instruciton length...
  // C extentions gives us 2-byte instructoins
  // RV-64 has 4-byte instructions
  // + RISCV architecture allows instructions up to 8 bytes
  // It seems difficult to figure out the exact instruction length -
  // pc - 2 seems like a safe option for the purposes of stack tracing
  return pc - 2;
#else
  return pc - 1;
#endif
}

// StackTrace that owns the buffer used to store the addresses.
struct BufferedStackTrace : public StackTrace {
  uptr trace_buffer[kStackTraceMax];
  uptr top_frame_bp;  // Optional bp of a top frame.

  BufferedStackTrace() : StackTrace(trace_buffer, 0), top_frame_bp(0) {}

  void Init(const uptr *pcs, uptr cnt, uptr extra_top_pc = 0);
  void Unwind(u32 max_depth, uptr pc, uptr bp, void *context, uptr stack_top,
              uptr stack_bottom, bool request_fast_unwind);

  void Reset() {
    *static_cast<StackTrace *>(this) = StackTrace(trace_buffer, 0);
    top_frame_bp = 0;
  }

 private:
  void FastUnwindStack(uptr pc, uptr bp, uptr stack_top, uptr stack_bottom,
                       u32 max_depth);
  void SlowUnwindStack(uptr pc, u32 max_depth);
  void SlowUnwindStackWithContext(uptr pc, void *context,
                                  u32 max_depth);
  void PopStackFrames(uptr count);
  uptr LocatePcInTrace(uptr pc);

  BufferedStackTrace(const BufferedStackTrace &) = delete;
  void operator=(const BufferedStackTrace &) = delete;
};

#if defined(__s390x__)
static const uptr kFrameSize = 160;
#elif defined(__s390__)
static const uptr kFrameSize = 96;
#else
static const uptr kFrameSize = 2 * sizeof(uhwptr);
#endif

// Check if given pointer points into allocated stack area.
static inline bool IsValidFrame(uptr frame, uptr stack_top, uptr stack_bottom) {
  return frame > stack_bottom && frame < stack_top - kFrameSize;
}

}  // namespace __sanitizer

// Use this macro if you want to print stack trace with the caller
// of the current function in the top frame.
#define GET_CALLER_PC_BP_SP \
  uptr bp = GET_CURRENT_FRAME();              \
  uptr pc = GET_CALLER_PC();                  \
  uptr local_stack;                           \
  uptr sp = (uptr)&local_stack

#define GET_CALLER_PC_BP \
  uptr bp = GET_CURRENT_FRAME();              \
  uptr pc = GET_CALLER_PC();

// Use this macro if you want to print stack trace with the current
// function in the top frame.
#define GET_CURRENT_PC_BP_SP \
  uptr bp = GET_CURRENT_FRAME();              \
  uptr pc = StackTrace::GetCurrentPc();   \
  uptr local_stack;                           \
  uptr sp = (uptr)&local_stack

// GET_CURRENT_PC() is equivalent to StackTrace::GetCurrentPc().
// Optimized x86 version is faster than GetCurrentPc because
// it does not involve a function call, instead it reads RIP register.
// Reads of RIP by an instruction return RIP pointing to the next
// instruction, which is exactly what we want here, thus 0 offset.
// It needs to be a macro because otherwise we will get the name
// of this function on the top of most stacks. Attribute artificial
// does not do what it claims to do, unfortunatley. And attribute
// __nodebug__ is clang-only. If we would have an attribute that
// would remove this function from debug info, we could simply make
// StackTrace::GetCurrentPc() faster.
#if defined(__x86_64__)
#  define GET_CURRENT_PC()                \
    (__extension__({                      \
      uptr pc;                            \
      asm("lea 0(%%rip), %0" : "=r"(pc)); \
      pc;                                 \
    }))
#else
#  define GET_CURRENT_PC() StackTrace::GetCurrentPc()
#endif

#endif  // SANITIZER_STACKTRACE_H
