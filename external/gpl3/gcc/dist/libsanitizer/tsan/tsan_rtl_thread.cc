//===-- tsan_rtl_thread.cc ------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_placement_new.h"
#include "tsan_rtl.h"
#include "tsan_mman.h"
#include "tsan_platform.h"
#include "tsan_report.h"
#include "tsan_sync.h"

namespace __tsan {

#ifndef TSAN_GO
const int kThreadQuarantineSize = 16;
#else
const int kThreadQuarantineSize = 64;
#endif

static void MaybeReportThreadLeak(ThreadContext *tctx) {
  if (tctx->detached)
    return;
  if (tctx->status != ThreadStatusCreated
      && tctx->status != ThreadStatusRunning
      && tctx->status != ThreadStatusFinished)
    return;
  ScopedReport rep(ReportTypeThreadLeak);
  rep.AddThread(tctx);
  OutputReport(CTX(), rep);
}

void ThreadFinalize(ThreadState *thr) {
  CHECK_GT(thr->in_rtl, 0);
  if (!flags()->report_thread_leaks)
    return;
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  for (unsigned i = 0; i < kMaxTid; i++) {
    ThreadContext *tctx = ctx->threads[i];
    if (tctx == 0)
      continue;
    MaybeReportThreadLeak(tctx);
  }
}

int ThreadCount(ThreadState *thr) {
  CHECK_GT(thr->in_rtl, 0);
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  int cnt = 0;
  for (unsigned i = 0; i < kMaxTid; i++) {
    ThreadContext *tctx = ctx->threads[i];
    if (tctx == 0)
      continue;
    if (tctx->status != ThreadStatusCreated
        && tctx->status != ThreadStatusRunning)
      continue;
    cnt++;
  }
  return cnt;
}

static void ThreadDead(ThreadState *thr, ThreadContext *tctx) {
  Context *ctx = CTX();
  CHECK_GT(thr->in_rtl, 0);
  CHECK(tctx->status == ThreadStatusRunning
      || tctx->status == ThreadStatusFinished);
  DPrintf("#%d: ThreadDead uid=%zu\n", thr->tid, tctx->user_id);
  tctx->status = ThreadStatusDead;
  tctx->user_id = 0;
  tctx->sync.Reset();

  // Put to dead list.
  tctx->dead_next = 0;
  if (ctx->dead_list_size == 0)
    ctx->dead_list_head = tctx;
  else
    ctx->dead_list_tail->dead_next = tctx;
  ctx->dead_list_tail = tctx;
  ctx->dead_list_size++;
}

int ThreadCreate(ThreadState *thr, uptr pc, uptr uid, bool detached) {
  CHECK_GT(thr->in_rtl, 0);
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  StatInc(thr, StatThreadCreate);
  int tid = -1;
  ThreadContext *tctx = 0;
  if (ctx->dead_list_size > kThreadQuarantineSize
      || ctx->thread_seq >= kMaxTid) {
    // Reusing old thread descriptor and tid.
    if (ctx->dead_list_size == 0) {
      Printf("ThreadSanitizer: %d thread limit exceeded. Dying.\n",
                 kMaxTid);
      Die();
    }
    StatInc(thr, StatThreadReuse);
    tctx = ctx->dead_list_head;
    ctx->dead_list_head = tctx->dead_next;
    ctx->dead_list_size--;
    if (ctx->dead_list_size == 0) {
      CHECK_EQ(tctx->dead_next, 0);
      ctx->dead_list_head = 0;
    }
    CHECK_EQ(tctx->status, ThreadStatusDead);
    tctx->status = ThreadStatusInvalid;
    tctx->reuse_count++;
    tctx->sync.Reset();
    tid = tctx->tid;
    DestroyAndFree(tctx->dead_info);
    if (tctx->name) {
      internal_free(tctx->name);
      tctx->name = 0;
    }
  } else {
    // Allocating new thread descriptor and tid.
    StatInc(thr, StatThreadMaxTid);
    tid = ctx->thread_seq++;
    void *mem = internal_alloc(MBlockThreadContex, sizeof(ThreadContext));
    tctx = new(mem) ThreadContext(tid);
    ctx->threads[tid] = tctx;
    MapThreadTrace(GetThreadTrace(tid), TraceSize() * sizeof(Event));
  }
  CHECK_NE(tctx, 0);
  CHECK_GE(tid, 0);
  CHECK_LT(tid, kMaxTid);
  DPrintf("#%d: ThreadCreate tid=%d uid=%zu\n", thr->tid, tid, uid);
  CHECK_EQ(tctx->status, ThreadStatusInvalid);
  ctx->alive_threads++;
  if (ctx->max_alive_threads < ctx->alive_threads) {
    ctx->max_alive_threads++;
    CHECK_EQ(ctx->max_alive_threads, ctx->alive_threads);
    StatInc(thr, StatThreadMaxAlive);
  }
  tctx->status = ThreadStatusCreated;
  tctx->thr = 0;
  tctx->user_id = uid;
  tctx->unique_id = ctx->unique_thread_seq++;
  tctx->detached = detached;
  if (tid) {
    thr->fast_state.IncrementEpoch();
    // Can't increment epoch w/o writing to the trace as well.
    TraceAddEvent(thr, thr->fast_state, EventTypeMop, 0);
    thr->clock.set(thr->tid, thr->fast_state.epoch());
    thr->fast_synch_epoch = thr->fast_state.epoch();
    thr->clock.release(&tctx->sync);
    StatInc(thr, StatSyncRelease);
    tctx->creation_stack.ObtainCurrent(thr, pc);
    tctx->creation_tid = thr->tid;
  }
  return tid;
}

void ThreadStart(ThreadState *thr, int tid, uptr os_id) {
  CHECK_GT(thr->in_rtl, 0);
  uptr stk_addr = 0;
  uptr stk_size = 0;
  uptr tls_addr = 0;
  uptr tls_size = 0;
  GetThreadStackAndTls(tid == 0, &stk_addr, &stk_size, &tls_addr, &tls_size);

  if (tid) {
    if (stk_addr && stk_size) {
      MemoryResetRange(thr, /*pc=*/ 1, stk_addr, stk_size);
    }

    if (tls_addr && tls_size) {
      // Check that the thr object is in tls;
      const uptr thr_beg = (uptr)thr;
      const uptr thr_end = (uptr)thr + sizeof(*thr);
      CHECK_GE(thr_beg, tls_addr);
      CHECK_LE(thr_beg, tls_addr + tls_size);
      CHECK_GE(thr_end, tls_addr);
      CHECK_LE(thr_end, tls_addr + tls_size);
      // Since the thr object is huge, skip it.
      MemoryResetRange(thr, /*pc=*/ 2, tls_addr, thr_beg - tls_addr);
      MemoryResetRange(thr, /*pc=*/ 2, thr_end, tls_addr + tls_size - thr_end);
    }
  }

  Lock l(&CTX()->thread_mtx);
  ThreadContext *tctx = CTX()->threads[tid];
  CHECK_NE(tctx, 0);
  CHECK_EQ(tctx->status, ThreadStatusCreated);
  tctx->status = ThreadStatusRunning;
  tctx->os_id = os_id;
  // RoundUp so that one trace part does not contain events
  // from different threads.
  tctx->epoch0 = RoundUp(tctx->epoch1 + 1, kTracePartSize);
  tctx->epoch1 = (u64)-1;
  new(thr) ThreadState(CTX(), tid, tctx->unique_id,
      tctx->epoch0, stk_addr, stk_size,
      tls_addr, tls_size);
#ifdef TSAN_GO
  // Setup dynamic shadow stack.
  const int kInitStackSize = 8;
  thr->shadow_stack = (uptr*)internal_alloc(MBlockShadowStack,
      kInitStackSize * sizeof(uptr));
  thr->shadow_stack_pos = thr->shadow_stack;
  thr->shadow_stack_end = thr->shadow_stack + kInitStackSize;
#endif
#ifndef TSAN_GO
  AllocatorThreadStart(thr);
#endif
  tctx->thr = thr;
  thr->fast_synch_epoch = tctx->epoch0;
  thr->clock.set(tid, tctx->epoch0);
  thr->clock.acquire(&tctx->sync);
  thr->fast_state.SetHistorySize(flags()->history_size);
  const uptr trace = (tctx->epoch0 / kTracePartSize) % TraceParts();
  thr->trace.headers[trace].epoch0 = tctx->epoch0;
  StatInc(thr, StatSyncAcquire);
  DPrintf("#%d: ThreadStart epoch=%zu stk_addr=%zx stk_size=%zx "
          "tls_addr=%zx tls_size=%zx\n",
          tid, (uptr)tctx->epoch0, stk_addr, stk_size, tls_addr, tls_size);
  thr->is_alive = true;
}

void ThreadFinish(ThreadState *thr) {
  CHECK_GT(thr->in_rtl, 0);
  StatInc(thr, StatThreadFinish);
  // FIXME: Treat it as write.
  if (thr->stk_addr && thr->stk_size)
    MemoryResetRange(thr, /*pc=*/ 3, thr->stk_addr, thr->stk_size);
  if (thr->tls_addr && thr->tls_size) {
    const uptr thr_beg = (uptr)thr;
    const uptr thr_end = (uptr)thr + sizeof(*thr);
    // Since the thr object is huge, skip it.
    MemoryResetRange(thr, /*pc=*/ 4, thr->tls_addr, thr_beg - thr->tls_addr);
    MemoryResetRange(thr, /*pc=*/ 5,
        thr_end, thr->tls_addr + thr->tls_size - thr_end);
  }
  thr->is_alive = false;
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  ThreadContext *tctx = ctx->threads[thr->tid];
  CHECK_NE(tctx, 0);
  CHECK_EQ(tctx->status, ThreadStatusRunning);
  CHECK_GT(ctx->alive_threads, 0);
  ctx->alive_threads--;
  if (tctx->detached) {
    ThreadDead(thr, tctx);
  } else {
    thr->fast_state.IncrementEpoch();
    // Can't increment epoch w/o writing to the trace as well.
    TraceAddEvent(thr, thr->fast_state, EventTypeMop, 0);
    thr->clock.set(thr->tid, thr->fast_state.epoch());
    thr->fast_synch_epoch = thr->fast_state.epoch();
    thr->clock.release(&tctx->sync);
    StatInc(thr, StatSyncRelease);
    tctx->status = ThreadStatusFinished;
  }

  // Save from info about the thread.
  tctx->dead_info = new(internal_alloc(MBlockDeadInfo, sizeof(ThreadDeadInfo)))
      ThreadDeadInfo();
  for (uptr i = 0; i < TraceParts(); i++) {
    tctx->dead_info->trace.headers[i].epoch0 = thr->trace.headers[i].epoch0;
    tctx->dead_info->trace.headers[i].stack0.CopyFrom(
        thr->trace.headers[i].stack0);
  }
  tctx->epoch1 = thr->fast_state.epoch();

#ifndef TSAN_GO
  AllocatorThreadFinish(thr);
#endif
  thr->~ThreadState();
  StatAggregate(ctx->stat, thr->stat);
  tctx->thr = 0;
}

int ThreadTid(ThreadState *thr, uptr pc, uptr uid) {
  CHECK_GT(thr->in_rtl, 0);
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  int res = -1;
  for (unsigned tid = 0; tid < kMaxTid; tid++) {
    ThreadContext *tctx = ctx->threads[tid];
    if (tctx != 0 && tctx->user_id == uid
        && tctx->status != ThreadStatusInvalid) {
      tctx->user_id = 0;
      res = tid;
      break;
    }
  }
  DPrintf("#%d: ThreadTid uid=%zu tid=%d\n", thr->tid, uid, res);
  return res;
}

void ThreadJoin(ThreadState *thr, uptr pc, int tid) {
  CHECK_GT(thr->in_rtl, 0);
  CHECK_GT(tid, 0);
  CHECK_LT(tid, kMaxTid);
  DPrintf("#%d: ThreadJoin tid=%d\n", thr->tid, tid);
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  ThreadContext *tctx = ctx->threads[tid];
  if (tctx->status == ThreadStatusInvalid) {
    Printf("ThreadSanitizer: join of non-existent thread\n");
    return;
  }
  // FIXME(dvyukov): print message and continue (it's user error).
  CHECK_EQ(tctx->detached, false);
  CHECK_EQ(tctx->status, ThreadStatusFinished);
  thr->clock.acquire(&tctx->sync);
  StatInc(thr, StatSyncAcquire);
  ThreadDead(thr, tctx);
}

void ThreadDetach(ThreadState *thr, uptr pc, int tid) {
  CHECK_GT(thr->in_rtl, 0);
  CHECK_GT(tid, 0);
  CHECK_LT(tid, kMaxTid);
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  ThreadContext *tctx = ctx->threads[tid];
  if (tctx->status == ThreadStatusInvalid) {
    Printf("ThreadSanitizer: detach of non-existent thread\n");
    return;
  }
  if (tctx->status == ThreadStatusFinished) {
    ThreadDead(thr, tctx);
  } else {
    tctx->detached = true;
  }
}

void ThreadSetName(ThreadState *thr, const char *name) {
  Context *ctx = CTX();
  Lock l(&ctx->thread_mtx);
  ThreadContext *tctx = ctx->threads[thr->tid];
  CHECK_NE(tctx, 0);
  CHECK_EQ(tctx->status, ThreadStatusRunning);
  if (tctx->name) {
    internal_free(tctx->name);
    tctx->name = 0;
  }
  if (name)
    tctx->name = internal_strdup(name);
}

void MemoryAccessRange(ThreadState *thr, uptr pc, uptr addr,
                       uptr size, bool is_write) {
  if (size == 0)
    return;

  u64 *shadow_mem = (u64*)MemToShadow(addr);
  DPrintf2("#%d: MemoryAccessRange: @%p %p size=%d is_write=%d\n",
      thr->tid, (void*)pc, (void*)addr,
      (int)size, is_write);

#if TSAN_DEBUG
  if (!IsAppMem(addr)) {
    Printf("Access to non app mem %zx\n", addr);
    DCHECK(IsAppMem(addr));
  }
  if (!IsAppMem(addr + size - 1)) {
    Printf("Access to non app mem %zx\n", addr + size - 1);
    DCHECK(IsAppMem(addr + size - 1));
  }
  if (!IsShadowMem((uptr)shadow_mem)) {
    Printf("Bad shadow addr %p (%zx)\n", shadow_mem, addr);
    DCHECK(IsShadowMem((uptr)shadow_mem));
  }
  if (!IsShadowMem((uptr)(shadow_mem + size * kShadowCnt / 8 - 1))) {
    Printf("Bad shadow addr %p (%zx)\n",
               shadow_mem + size * kShadowCnt / 8 - 1, addr + size - 1);
    DCHECK(IsShadowMem((uptr)(shadow_mem + size * kShadowCnt / 8 - 1)));
  }
#endif

  StatInc(thr, StatMopRange);

  FastState fast_state = thr->fast_state;
  if (fast_state.GetIgnoreBit())
    return;

  fast_state.IncrementEpoch();
  thr->fast_state = fast_state;
  TraceAddEvent(thr, fast_state, EventTypeMop, pc);

  bool unaligned = (addr % kShadowCell) != 0;

  // Handle unaligned beginning, if any.
  for (; addr % kShadowCell && size; addr++, size--) {
    int const kAccessSizeLog = 0;
    Shadow cur(fast_state);
    cur.SetWrite(is_write);
    cur.SetAddr0AndSizeLog(addr & (kShadowCell - 1), kAccessSizeLog);
    MemoryAccessImpl(thr, addr, kAccessSizeLog, is_write, false,
        shadow_mem, cur);
  }
  if (unaligned)
    shadow_mem += kShadowCnt;
  // Handle middle part, if any.
  for (; size >= kShadowCell; addr += kShadowCell, size -= kShadowCell) {
    int const kAccessSizeLog = 3;
    Shadow cur(fast_state);
    cur.SetWrite(is_write);
    cur.SetAddr0AndSizeLog(0, kAccessSizeLog);
    MemoryAccessImpl(thr, addr, kAccessSizeLog, is_write, false,
        shadow_mem, cur);
    shadow_mem += kShadowCnt;
  }
  // Handle ending, if any.
  for (; size; addr++, size--) {
    int const kAccessSizeLog = 0;
    Shadow cur(fast_state);
    cur.SetWrite(is_write);
    cur.SetAddr0AndSizeLog(addr & (kShadowCell - 1), kAccessSizeLog);
    MemoryAccessImpl(thr, addr, kAccessSizeLog, is_write, false,
        shadow_mem, cur);
  }
}

void MemoryAccessRangeStep(ThreadState *thr, uptr pc, uptr addr,
    uptr size, uptr step, bool is_write) {
  if (size == 0)
    return;
  FastState fast_state = thr->fast_state;
  if (fast_state.GetIgnoreBit())
    return;
  StatInc(thr, StatMopRange);
  fast_state.IncrementEpoch();
  thr->fast_state = fast_state;
  TraceAddEvent(thr, fast_state, EventTypeMop, pc);

  for (uptr addr_end = addr + size; addr < addr_end; addr += step) {
    u64 *shadow_mem = (u64*)MemToShadow(addr);
    Shadow cur(fast_state);
    cur.SetWrite(is_write);
    cur.SetAddr0AndSizeLog(addr & (kShadowCell - 1), kSizeLog1);
    MemoryAccessImpl(thr, addr, kSizeLog1, is_write, false,
        shadow_mem, cur);
  }
}
}  // namespace __tsan
