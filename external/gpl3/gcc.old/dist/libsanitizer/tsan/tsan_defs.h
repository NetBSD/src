//===-- tsan_defs.h ---------------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_DEFS_H
#define TSAN_DEFS_H

#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "tsan_stat.h"
#include "ubsan/ubsan_platform.h"

// Setup defaults for compile definitions.
#ifndef TSAN_NO_HISTORY
# define TSAN_NO_HISTORY 0
#endif

#ifndef TSAN_COLLECT_STATS
# define TSAN_COLLECT_STATS 0
#endif

#ifndef TSAN_CONTAINS_UBSAN
# define TSAN_CONTAINS_UBSAN (CAN_SANITIZE_UB && !defined(SANITIZER_GO))
#endif

namespace __tsan {

#ifdef SANITIZER_GO
const bool kGoMode = true;
const bool kCppMode = false;
const char *const kTsanOptionsEnv = "GORACE";
// Go linker does not support weak symbols.
#define CPP_WEAK
#else
const bool kGoMode = false;
const bool kCppMode = true;
const char *const kTsanOptionsEnv = "TSAN_OPTIONS";
#define CPP_WEAK WEAK
#endif

const int kTidBits = 13;
const unsigned kMaxTid = 1 << kTidBits;
#ifndef SANITIZER_GO
const unsigned kMaxTidInClock = kMaxTid * 2;  // This includes msb 'freed' bit.
#else
const unsigned kMaxTidInClock = kMaxTid;  // Go does not track freed memory.
#endif
const int kClkBits = 42;
const unsigned kMaxTidReuse = (1 << (64 - kClkBits)) - 1;
const uptr kShadowStackSize = 64 * 1024;

// Count of shadow values in a shadow cell.
const uptr kShadowCnt = 4;

// That many user bytes are mapped onto a single shadow cell.
const uptr kShadowCell = 8;

// Size of a single shadow value (u64).
const uptr kShadowSize = 8;

// Shadow memory is kShadowMultiplier times larger than user memory.
const uptr kShadowMultiplier = kShadowSize * kShadowCnt / kShadowCell;

// That many user bytes are mapped onto a single meta shadow cell.
// Must be less or equal to minimal memory allocator alignment.
const uptr kMetaShadowCell = 8;

// Size of a single meta shadow value (u32).
const uptr kMetaShadowSize = 4;

#if TSAN_NO_HISTORY
const bool kCollectHistory = false;
#else
const bool kCollectHistory = true;
#endif

const unsigned kInvalidTid = (unsigned)-1;

// The following "build consistency" machinery ensures that all source files
// are built in the same configuration. Inconsistent builds lead to
// hard to debug crashes.
#if SANITIZER_DEBUG
void build_consistency_debug();
#else
void build_consistency_release();
#endif

#if TSAN_COLLECT_STATS
void build_consistency_stats();
#else
void build_consistency_nostats();
#endif

static inline void USED build_consistency() {
#if SANITIZER_DEBUG
  build_consistency_debug();
#else
  build_consistency_release();
#endif
#if TSAN_COLLECT_STATS
  build_consistency_stats();
#else
  build_consistency_nostats();
#endif
}

template<typename T>
T min(T a, T b) {
  return a < b ? a : b;
}

template<typename T>
T max(T a, T b) {
  return a > b ? a : b;
}

template<typename T>
T RoundUp(T p, u64 align) {
  DCHECK_EQ(align & (align - 1), 0);
  return (T)(((u64)p + align - 1) & ~(align - 1));
}

template<typename T>
T RoundDown(T p, u64 align) {
  DCHECK_EQ(align & (align - 1), 0);
  return (T)((u64)p & ~(align - 1));
}

// Zeroizes high part, returns 'bits' lsb bits.
template<typename T>
T GetLsb(T v, int bits) {
  return (T)((u64)v & ((1ull << bits) - 1));
}

struct MD5Hash {
  u64 hash[2];
  bool operator==(const MD5Hash &other) const;
};

MD5Hash md5_hash(const void *data, uptr size);

struct ThreadState;
class ThreadContext;
struct Context;
struct ReportStack;
class ReportDesc;
class RegionAlloc;

// Descriptor of user's memory block.
struct MBlock {
  u64  siz;
  u32  stk;
  u16  tid;
};

COMPILER_CHECK(sizeof(MBlock) == 16);

}  // namespace __tsan

#endif  // TSAN_DEFS_H
