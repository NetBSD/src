//===-- sanitizer_atomic_msvc.h ---------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer/AddressSanitizer runtime.
// Not intended for direct inclusion. Include sanitizer_atomic.h.
//
//===----------------------------------------------------------------------===//

#ifndef SANITIZER_ATOMIC_MSVC_H
#define SANITIZER_ATOMIC_MSVC_H

extern "C" void _ReadWriteBarrier();
#pragma intrinsic(_ReadWriteBarrier)
extern "C" void _mm_mfence();
#pragma intrinsic(_mm_mfence)
extern "C" void _mm_pause();
#pragma intrinsic(_mm_pause)
extern "C" long _InterlockedExchangeAdd(  // NOLINT
    long volatile * Addend, long Value);  // NOLINT
#pragma intrinsic(_InterlockedExchangeAdd)

#ifdef _WIN64
extern "C" void *_InterlockedCompareExchangePointer(
    void *volatile *Destination,
    void *Exchange, void *Comparand);
#pragma intrinsic(_InterlockedCompareExchangePointer)
#else
// There's no _InterlockedCompareExchangePointer intrinsic on x86,
// so call _InterlockedCompareExchange instead.
extern "C"
long __cdecl _InterlockedCompareExchange(  // NOLINT
    long volatile *Destination,            // NOLINT
    long Exchange, long Comparand);        // NOLINT
#pragma intrinsic(_InterlockedCompareExchange)

inline static void *_InterlockedCompareExchangePointer(
    void *volatile *Destination,
    void *Exchange, void *Comparand) {
  return reinterpret_cast<void*>(
      _InterlockedCompareExchange(
          reinterpret_cast<long volatile*>(Destination),  // NOLINT
          reinterpret_cast<long>(Exchange),               // NOLINT
          reinterpret_cast<long>(Comparand)));            // NOLINT
}
#endif

namespace __sanitizer {

INLINE void atomic_signal_fence(memory_order) {
  _ReadWriteBarrier();
}

INLINE void atomic_thread_fence(memory_order) {
  _mm_mfence();
}

INLINE void proc_yield(int cnt) {
  for (int i = 0; i < cnt; i++)
    _mm_pause();
}

template<typename T>
INLINE typename T::Type atomic_load(
    const volatile T *a, memory_order mo) {
  DCHECK(mo & (memory_order_relaxed | memory_order_consume
      | memory_order_acquire | memory_order_seq_cst));
  DCHECK(!((uptr)a % sizeof(*a)));
  typename T::Type v;
  // FIXME(dvyukov): 64-bit load is not atomic on 32-bits.
  if (mo == memory_order_relaxed) {
    v = a->val_dont_use;
  } else {
    atomic_signal_fence(memory_order_seq_cst);
    v = a->val_dont_use;
    atomic_signal_fence(memory_order_seq_cst);
  }
  return v;
}

template<typename T>
INLINE void atomic_store(volatile T *a, typename T::Type v, memory_order mo) {
  DCHECK(mo & (memory_order_relaxed | memory_order_release
      | memory_order_seq_cst));
  DCHECK(!((uptr)a % sizeof(*a)));
  // FIXME(dvyukov): 64-bit store is not atomic on 32-bits.
  if (mo == memory_order_relaxed) {
    a->val_dont_use = v;
  } else {
    atomic_signal_fence(memory_order_seq_cst);
    a->val_dont_use = v;
    atomic_signal_fence(memory_order_seq_cst);
  }
  if (mo == memory_order_seq_cst)
    atomic_thread_fence(memory_order_seq_cst);
}

INLINE u32 atomic_fetch_add(volatile atomic_uint32_t *a,
    u32 v, memory_order mo) {
  (void)mo;
  DCHECK(!((uptr)a % sizeof(*a)));
  return (u32)_InterlockedExchangeAdd(
      (volatile long*)&a->val_dont_use, (long)v);  // NOLINT
}

INLINE u8 atomic_exchange(volatile atomic_uint8_t *a,
    u8 v, memory_order mo) {
  (void)mo;
  DCHECK(!((uptr)a % sizeof(*a)));
  __asm {
    mov eax, a
    mov cl, v
    xchg [eax], cl  // NOLINT
    mov v, cl
  }
  return v;
}

INLINE u16 atomic_exchange(volatile atomic_uint16_t *a,
    u16 v, memory_order mo) {
  (void)mo;
  DCHECK(!((uptr)a % sizeof(*a)));
  __asm {
    mov eax, a
    mov cx, v
    xchg [eax], cx  // NOLINT
    mov v, cx
  }
  return v;
}

INLINE bool atomic_compare_exchange_strong(volatile atomic_uintptr_t *a,
                                           uptr *cmp,
                                           uptr xchg,
                                           memory_order mo) {
  uptr cmpv = *cmp;
  uptr prev = (uptr)_InterlockedCompareExchangePointer(
      (void*volatile*)&a->val_dont_use, (void*)xchg, (void*)cmpv);
  if (prev == cmpv)
    return true;
  *cmp = prev;
  return false;
}

template<typename T>
INLINE bool atomic_compare_exchange_weak(volatile T *a,
                                           typename T::Type *cmp,
                                           typename T::Type xchg,
                                           memory_order mo) {
  return atomic_compare_exchange_strong(a, cmp, xchg, mo);
}

}  // namespace __sanitizer

#endif  // SANITIZER_ATOMIC_CLANG_H
