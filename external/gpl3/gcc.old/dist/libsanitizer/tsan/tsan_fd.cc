//===-- tsan_fd.cc --------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//

#include "tsan_fd.h"
#include "tsan_rtl.h"
#include <sanitizer_common/sanitizer_atomic.h>

namespace __tsan {

const int kTableSizeL1 = 1024;
const int kTableSizeL2 = 1024;
const int kTableSize = kTableSizeL1 * kTableSizeL2;

struct FdSync {
  atomic_uint64_t rc;
};

struct FdDesc {
  FdSync *sync;
  int creation_tid;
  u32 creation_stack;
};

struct FdContext {
  atomic_uintptr_t tab[kTableSizeL1];
  // Addresses used for synchronization.
  FdSync globsync;
  FdSync filesync;
  FdSync socksync;
  u64 connectsync;
};

static FdContext fdctx;

static FdSync *allocsync() {
  FdSync *s = (FdSync*)internal_alloc(MBlockFD, sizeof(FdSync));
  atomic_store(&s->rc, 1, memory_order_relaxed);
  return s;
}

static FdSync *ref(FdSync *s) {
  if (s && atomic_load(&s->rc, memory_order_relaxed) != (u64)-1)
    atomic_fetch_add(&s->rc, 1, memory_order_relaxed);
  return s;
}

static void unref(ThreadState *thr, uptr pc, FdSync *s) {
  if (s && atomic_load(&s->rc, memory_order_relaxed) != (u64)-1) {
    if (atomic_fetch_sub(&s->rc, 1, memory_order_acq_rel) == 1) {
      CHECK_NE(s, &fdctx.globsync);
      CHECK_NE(s, &fdctx.filesync);
      CHECK_NE(s, &fdctx.socksync);
      SyncVar *v = CTX()->synctab.GetAndRemove(thr, pc, (uptr)s);
      if (v)
        DestroyAndFree(v);
      internal_free(s);
    }
  }
}

static FdDesc *fddesc(ThreadState *thr, uptr pc, int fd) {
  CHECK_LT(fd, kTableSize);
  atomic_uintptr_t *pl1 = &fdctx.tab[fd / kTableSizeL2];
  uptr l1 = atomic_load(pl1, memory_order_consume);
  if (l1 == 0) {
    uptr size = kTableSizeL2 * sizeof(FdDesc);
    void *p = internal_alloc(MBlockFD, size);
    internal_memset(p, 0, size);
    MemoryResetRange(thr, (uptr)&fddesc, (uptr)p, size);
    if (atomic_compare_exchange_strong(pl1, &l1, (uptr)p, memory_order_acq_rel))
      l1 = (uptr)p;
    else
      internal_free(p);
  }
  return &((FdDesc*)l1)[fd % kTableSizeL2];  // NOLINT
}

// pd must be already ref'ed.
static void init(ThreadState *thr, uptr pc, int fd, FdSync *s) {
  FdDesc *d = fddesc(thr, pc, fd);
  // As a matter of fact, we don't intercept all close calls.
  // See e.g. libc __res_iclose().
  if (d->sync) {
    unref(thr, pc, d->sync);
    d->sync = 0;
  }
  if (flags()->io_sync == 0) {
    unref(thr, pc, s);
  } else if (flags()->io_sync == 1) {
    d->sync = s;
  } else if (flags()->io_sync == 2) {
    unref(thr, pc, s);
    d->sync = &fdctx.globsync;
  }
  d->creation_tid = thr->tid;
  d->creation_stack = CurrentStackId(thr, pc);
  // To catch races between fd usage and open.
  MemoryRangeImitateWrite(thr, pc, (uptr)d, 8);
}

void FdInit() {
  atomic_store(&fdctx.globsync.rc, (u64)-1, memory_order_relaxed);
  atomic_store(&fdctx.filesync.rc, (u64)-1, memory_order_relaxed);
  atomic_store(&fdctx.socksync.rc, (u64)-1, memory_order_relaxed);
}

void FdOnFork(ThreadState *thr, uptr pc) {
  // On fork() we need to reset all fd's, because the child is going
  // close all them, and that will cause races between previous read/write
  // and the close.
  for (int l1 = 0; l1 < kTableSizeL1; l1++) {
    FdDesc *tab = (FdDesc*)atomic_load(&fdctx.tab[l1], memory_order_relaxed);
    if (tab == 0)
      break;
    for (int l2 = 0; l2 < kTableSizeL2; l2++) {
      FdDesc *d = &tab[l2];
      MemoryResetRange(thr, pc, (uptr)d, 8);
    }
  }
}

bool FdLocation(uptr addr, int *fd, int *tid, u32 *stack) {
  for (int l1 = 0; l1 < kTableSizeL1; l1++) {
    FdDesc *tab = (FdDesc*)atomic_load(&fdctx.tab[l1], memory_order_relaxed);
    if (tab == 0)
      break;
    if (addr >= (uptr)tab && addr < (uptr)(tab + kTableSizeL2)) {
      int l2 = (addr - (uptr)tab) / sizeof(FdDesc);
      FdDesc *d = &tab[l2];
      *fd = l1 * kTableSizeL1 + l2;
      *tid = d->creation_tid;
      *stack = d->creation_stack;
      return true;
    }
  }
  return false;
}

void FdAcquire(ThreadState *thr, uptr pc, int fd) {
  FdDesc *d = fddesc(thr, pc, fd);
  FdSync *s = d->sync;
  DPrintf("#%d: FdAcquire(%d) -> %p\n", thr->tid, fd, s);
  MemoryRead(thr, pc, (uptr)d, kSizeLog8);
  if (s)
    Acquire(thr, pc, (uptr)s);
}

void FdRelease(ThreadState *thr, uptr pc, int fd) {
  FdDesc *d = fddesc(thr, pc, fd);
  FdSync *s = d->sync;
  DPrintf("#%d: FdRelease(%d) -> %p\n", thr->tid, fd, s);
  if (s)
    Release(thr, pc, (uptr)s);
  MemoryRead(thr, pc, (uptr)d, kSizeLog8);
}

void FdAccess(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdAccess(%d)\n", thr->tid, fd);
  FdDesc *d = fddesc(thr, pc, fd);
  MemoryRead(thr, pc, (uptr)d, kSizeLog8);
}

void FdClose(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdClose(%d)\n", thr->tid, fd);
  FdDesc *d = fddesc(thr, pc, fd);
  // To catch races between fd usage and close.
  MemoryWrite(thr, pc, (uptr)d, kSizeLog8);
  // We need to clear it, because if we do not intercept any call out there
  // that creates fd, we will hit false postives.
  MemoryResetRange(thr, pc, (uptr)d, 8);
  unref(thr, pc, d->sync);
  d->sync = 0;
  d->creation_tid = 0;
  d->creation_stack = 0;
}

void FdFileCreate(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdFileCreate(%d)\n", thr->tid, fd);
  init(thr, pc, fd, &fdctx.filesync);
}

void FdDup(ThreadState *thr, uptr pc, int oldfd, int newfd) {
  DPrintf("#%d: FdDup(%d, %d)\n", thr->tid, oldfd, newfd);
  // Ignore the case when user dups not yet connected socket.
  FdDesc *od = fddesc(thr, pc, oldfd);
  MemoryRead(thr, pc, (uptr)od, kSizeLog8);
  FdClose(thr, pc, newfd);
  init(thr, pc, newfd, ref(od->sync));
}

void FdPipeCreate(ThreadState *thr, uptr pc, int rfd, int wfd) {
  DPrintf("#%d: FdCreatePipe(%d, %d)\n", thr->tid, rfd, wfd);
  FdSync *s = allocsync();
  init(thr, pc, rfd, ref(s));
  init(thr, pc, wfd, ref(s));
  unref(thr, pc, s);
}

void FdEventCreate(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdEventCreate(%d)\n", thr->tid, fd);
  init(thr, pc, fd, allocsync());
}

void FdSignalCreate(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdSignalCreate(%d)\n", thr->tid, fd);
  init(thr, pc, fd, 0);
}

void FdInotifyCreate(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdInotifyCreate(%d)\n", thr->tid, fd);
  init(thr, pc, fd, 0);
}

void FdPollCreate(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdPollCreate(%d)\n", thr->tid, fd);
  init(thr, pc, fd, allocsync());
}

void FdSocketCreate(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdSocketCreate(%d)\n", thr->tid, fd);
  // It can be a UDP socket.
  init(thr, pc, fd, &fdctx.socksync);
}

void FdSocketAccept(ThreadState *thr, uptr pc, int fd, int newfd) {
  DPrintf("#%d: FdSocketAccept(%d, %d)\n", thr->tid, fd, newfd);
  // Synchronize connect->accept.
  Acquire(thr, pc, (uptr)&fdctx.connectsync);
  init(thr, pc, newfd, &fdctx.socksync);
}

void FdSocketConnecting(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdSocketConnecting(%d)\n", thr->tid, fd);
  // Synchronize connect->accept.
  Release(thr, pc, (uptr)&fdctx.connectsync);
}

void FdSocketConnect(ThreadState *thr, uptr pc, int fd) {
  DPrintf("#%d: FdSocketConnect(%d)\n", thr->tid, fd);
  init(thr, pc, fd, &fdctx.socksync);
}

uptr File2addr(char *path) {
  (void)path;
  static u64 addr;
  return (uptr)&addr;
}

uptr Dir2addr(char *path) {
  (void)path;
  static u64 addr;
  return (uptr)&addr;
}

}  //  namespace __tsan
