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

#ifndef TSAN_DEBUG
#define TSAN_DEBUG 0
#endif  // TSAN_DEBUG

namespace __tsan {

#ifdef TSAN_GO
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
const unsigned kMaxTidInClock = kMaxTid * 2;  // This includes msb 'freed' bit.
const int kClkBits = 42;
#ifndef TSAN_GO
const int kShadowStackSize = 4 * 1024;
const int kTraceStackSize = 256;
#endif

#ifdef TSAN_SHADOW_COUNT
# if TSAN_SHADOW_COUNT == 2 \
  || TSAN_SHADOW_COUNT == 4 || TSAN_SHADOW_COUNT == 8
const uptr kShadowCnt = TSAN_SHADOW_COUNT;
# else
#   error "TSAN_SHADOW_COUNT must be one of 2,4,8"
# endif
#else
// Count of shadow values in a shadow cell.
const uptr kShadowCnt = 4;
#endif

// That many user bytes are mapped onto a single shadow cell.
const uptr kShadowCell = 8;

// Size of a single shadow value (u64).
const uptr kShadowSize = 8;

// Shadow memory is kShadowMultiplier times larger than user memory.
const uptr kShadowMultiplier = kShadowSize * kShadowCnt / kShadowCell;

#if defined(TSAN_COLLECT_STATS) && TSAN_COLLECT_STATS
const bool kCollectStats = true;
#else
const bool kCollectStats = false;
#endif

// The following "build consistency" machinery ensures that all source files
// are built in the same configuration. Inconsistent builds lead to
// hard to debug crashes.
#if TSAN_DEBUG
void build_consistency_debug();
#else
void build_consistency_release();
#endif

#if TSAN_COLLECT_STATS
void build_consistency_stats();
#else
void build_consistency_nostats();
#endif

#if TSAN_SHADOW_COUNT == 1
void build_consistency_shadow1();
#elif TSAN_SHADOW_COUNT == 2
void build_consistency_shadow2();
#elif TSAN_SHADOW_COUNT == 4
void build_consistency_shadow4();
#else
void build_consistency_shadow8();
#endif

static inline void USED build_consistency() {
#if TSAN_DEBUG
  build_consistency_debug();
#else
  build_consistency_release();
#endif
#if TSAN_COLLECT_STATS
  build_consistency_stats();
#else
  build_consistency_nostats();
#endif
#if TSAN_SHADOW_COUNT == 1
  build_consistency_shadow1();
#elif TSAN_SHADOW_COUNT == 2
  build_consistency_shadow2();
#elif TSAN_SHADOW_COUNT == 4
  build_consistency_shadow4();
#else
  build_consistency_shadow8();
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
struct ThreadContext;
struct Context;
struct ReportStack;
class ReportDesc;
class RegionAlloc;
class StackTrace;
struct MBlock;

}  // namespace __tsan

#endif  // TSAN_DEFS_H
