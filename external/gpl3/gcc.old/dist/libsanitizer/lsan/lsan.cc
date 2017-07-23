//=-- lsan.cc -------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of LeakSanitizer.
// Standalone LSan RTL.
//
//===----------------------------------------------------------------------===//

#include "lsan.h"

#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "lsan_allocator.h"
#include "lsan_common.h"
#include "lsan_thread.h"

bool lsan_inited;
bool lsan_init_is_running;

namespace __lsan {

///// Interface to the common LSan module. /////
bool WordIsPoisoned(uptr addr) {
  return false;
}

}  // namespace __lsan

using namespace __lsan;  // NOLINT

extern "C" void __lsan_init() {
  CHECK(!lsan_init_is_running);
  if (lsan_inited)
    return;
  lsan_init_is_running = true;
  SanitizerToolName = "LeakSanitizer";
  InitCommonLsan(true);
  InitializeAllocator();
  InitTlsSize();
  InitializeInterceptors();
  InitializeThreadRegistry();
  u32 tid = ThreadCreate(0, 0, true);
  CHECK_EQ(tid, 0);
  ThreadStart(tid, GetTid());
  SetCurrentThread(tid);

  if (common_flags()->detect_leaks && common_flags()->leak_check_at_exit)
    Atexit(DoLeakCheck);
  lsan_inited = true;
  lsan_init_is_running = false;
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __sanitizer_print_stack_trace() {
  GET_STACK_TRACE_FATAL;
  stack.Print();
}
