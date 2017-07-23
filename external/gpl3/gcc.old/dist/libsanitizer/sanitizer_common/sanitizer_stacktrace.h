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

#include "sanitizer_internal_defs.h"

namespace __sanitizer {

static const uptr kStackTraceMax = 256;

#if SANITIZER_LINUX && (defined(__aarch64__) || defined(__powerpc__) || \
                        defined(__powerpc64__) || defined(__sparc__) || \
                        defined(__mips__))
# define SANITIZER_CAN_FAST_UNWIND 0
#elif SANITIZER_WINDOWS
# define SANITIZER_CAN_FAST_UNWIND 0
#else
# define SANITIZER_CAN_FAST_UNWIND 1
#endif

// Fast unwind is the only option on Mac for now; we will need to
// revisit this macro when slow unwind works on Mac, see
// https://code.google.com/p/address-sanitizer/issues/detail?id=137
#if SANITIZER_MAC
# define SANITIZER_CAN_SLOW_UNWIND 0
#else
# define SANITIZER_CAN_SLOW_UNWIND 1
#endif

struct StackTrace {
  const uptr *trace;
  uptr size;

  StackTrace() : trace(nullptr), size(0) {}
  StackTrace(const uptr *trace, uptr size) : trace(trace), size(size) {}

  // Prints a symbolized stacktrace, followed by an empty line.
  void Print() const;

  static bool WillUseFastUnwind(bool request_fast_unwind) {
    if (!SANITIZER_CAN_FAST_UNWIND)
      return false;
    else if (!SANITIZER_CAN_SLOW_UNWIND)
      return true;
    return request_fast_unwind;
  }

  static uptr GetCurrentPc();
  static uptr GetPreviousInstructionPc(uptr pc);
  typedef bool (*SymbolizeCallback)(const void *pc, char *out_buffer,
                                    int out_size);
};

// StackTrace that owns the buffer used to store the addresses.
struct BufferedStackTrace : public StackTrace {
  uptr trace_buffer[kStackTraceMax];
  uptr top_frame_bp;  // Optional bp of a top frame.

  BufferedStackTrace() : StackTrace(trace_buffer, 0), top_frame_bp(0) {}

  void Init(const uptr *pcs, uptr cnt, uptr extra_top_pc = 0);
  void Unwind(uptr max_depth, uptr pc, uptr bp, void *context, uptr stack_top,
              uptr stack_bottom, bool request_fast_unwind);

 private:
  void FastUnwindStack(uptr pc, uptr bp, uptr stack_top, uptr stack_bottom,
                       uptr max_depth);
  void SlowUnwindStack(uptr pc, uptr max_depth);
  void SlowUnwindStackWithContext(uptr pc, void *context,
                                  uptr max_depth);
  void PopStackFrames(uptr count);
  uptr LocatePcInTrace(uptr pc);

  BufferedStackTrace(const BufferedStackTrace &);
  void operator=(const BufferedStackTrace &);
};

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


#endif  // SANITIZER_STACKTRACE_H
