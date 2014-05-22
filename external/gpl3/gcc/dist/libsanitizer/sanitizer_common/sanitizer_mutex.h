//===-- sanitizer_mutex.h ---------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer/AddressSanitizer runtime.
//
//===----------------------------------------------------------------------===//

#ifndef SANITIZER_MUTEX_H
#define SANITIZER_MUTEX_H

#include "sanitizer_atomic.h"
#include "sanitizer_internal_defs.h"
#include "sanitizer_libc.h"

namespace __sanitizer {

class StaticSpinMutex {
 public:
  void Init() {
    atomic_store(&state_, 0, memory_order_relaxed);
  }

  void Lock() {
    if (TryLock())
      return;
    LockSlow();
  }

  bool TryLock() {
    return atomic_exchange(&state_, 1, memory_order_acquire) == 0;
  }

  void Unlock() {
    atomic_store(&state_, 0, memory_order_release);
  }

 private:
  atomic_uint8_t state_;

  void NOINLINE LockSlow() {
    for (int i = 0;; i++) {
      if (i < 10)
        proc_yield(10);
      else
        internal_sched_yield();
      if (atomic_load(&state_, memory_order_relaxed) == 0
          && atomic_exchange(&state_, 1, memory_order_acquire) == 0)
        return;
    }
  }
};

class SpinMutex : public StaticSpinMutex {
 public:
  SpinMutex() {
    Init();
  }

 private:
  SpinMutex(const SpinMutex&);
  void operator=(const SpinMutex&);
};

class BlockingMutex {
 public:
  explicit BlockingMutex(LinkerInitialized);
  void Lock();
  void Unlock();
 private:
  uptr opaque_storage_[10];
  uptr owner_;  // for debugging
};

template<typename MutexType>
class GenericScopedLock {
 public:
  explicit GenericScopedLock(MutexType *mu)
      : mu_(mu) {
    mu_->Lock();
  }

  ~GenericScopedLock() {
    mu_->Unlock();
  }

 private:
  MutexType *mu_;

  GenericScopedLock(const GenericScopedLock&);
  void operator=(const GenericScopedLock&);
};

template<typename MutexType>
class GenericScopedReadLock {
 public:
  explicit GenericScopedReadLock(MutexType *mu)
      : mu_(mu) {
    mu_->ReadLock();
  }

  ~GenericScopedReadLock() {
    mu_->ReadUnlock();
  }

 private:
  MutexType *mu_;

  GenericScopedReadLock(const GenericScopedReadLock&);
  void operator=(const GenericScopedReadLock&);
};

typedef GenericScopedLock<StaticSpinMutex> SpinMutexLock;
typedef GenericScopedLock<BlockingMutex> BlockingMutexLock;

}  // namespace __sanitizer

#endif  // SANITIZER_MUTEX_H
