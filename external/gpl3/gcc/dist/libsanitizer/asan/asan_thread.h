//===-- asan_thread.h -------------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// ASan-private header for asan_thread.cc.
//===----------------------------------------------------------------------===//
#ifndef ASAN_THREAD_H
#define ASAN_THREAD_H

#include "asan_allocator.h"
#include "asan_internal.h"
#include "asan_stack.h"
#include "asan_stats.h"
#include "sanitizer_common/sanitizer_libc.h"

namespace __asan {

const u32 kInvalidTid = 0xffffff;  // Must fit into 24 bits.

class AsanThread;

// These objects are created for every thread and are never deleted,
// so we can find them by tid even if the thread is long dead.
class AsanThreadSummary {
 public:
  explicit AsanThreadSummary(LinkerInitialized) { }  // for T0.
  void Init(u32 parent_tid, StackTrace *stack) {
    parent_tid_ = parent_tid;
    announced_ = false;
    tid_ = kInvalidTid;
    if (stack) {
      internal_memcpy(&stack_, stack, sizeof(*stack));
    }
    thread_ = 0;
    name_[0] = 0;
  }
  u32 tid() { return tid_; }
  void set_tid(u32 tid) { tid_ = tid; }
  u32 parent_tid() { return parent_tid_; }
  bool announced() { return announced_; }
  void set_announced(bool announced) { announced_ = announced; }
  StackTrace *stack() { return &stack_; }
  AsanThread *thread() { return thread_; }
  void set_thread(AsanThread *thread) { thread_ = thread; }
  static void TSDDtor(void *tsd);
  void set_name(const char *name) {
    internal_strncpy(name_, name, sizeof(name_) - 1);
  }
  const char *name() { return name_; }

 private:
  u32 tid_;
  u32 parent_tid_;
  bool announced_;
  StackTrace stack_;
  AsanThread *thread_;
  char name_[128];
};

// AsanThreadSummary objects are never freed, so we need many of them.
COMPILER_CHECK(sizeof(AsanThreadSummary) <= 4094);

// AsanThread are stored in TSD and destroyed when the thread dies.
class AsanThread {
 public:
  explicit AsanThread(LinkerInitialized);  // for T0.
  static AsanThread *Create(u32 parent_tid, thread_callback_t start_routine,
                            void *arg, StackTrace *stack);
  void Destroy();

  void Init();  // Should be called from the thread itself.
  thread_return_t ThreadStart();

  uptr stack_top() { return stack_top_; }
  uptr stack_bottom() { return stack_bottom_; }
  uptr stack_size() { return stack_top_ - stack_bottom_; }
  u32 tid() { return summary_->tid(); }
  AsanThreadSummary *summary() { return summary_; }
  void set_summary(AsanThreadSummary *summary) { summary_ = summary; }

  const char *GetFrameNameByAddr(uptr addr, uptr *offset);

  bool AddrIsInStack(uptr addr) {
    return addr >= stack_bottom_ && addr < stack_top_;
  }

  FakeStack &fake_stack() { return fake_stack_; }
  AsanThreadLocalMallocStorage &malloc_storage() { return malloc_storage_; }
  AsanStats &stats() { return stats_; }

 private:
  void SetThreadStackTopAndBottom();
  void ClearShadowForThreadStack();
  AsanThreadSummary *summary_;
  thread_callback_t start_routine_;
  void *arg_;
  uptr  stack_top_;
  uptr  stack_bottom_;

  FakeStack fake_stack_;
  AsanThreadLocalMallocStorage malloc_storage_;
  AsanStats stats_;
};

}  // namespace __asan

#endif  // ASAN_THREAD_H
