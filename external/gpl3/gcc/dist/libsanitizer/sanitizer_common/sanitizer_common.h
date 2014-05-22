//===-- sanitizer_common.h --------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is shared between AddressSanitizer and ThreadSanitizer
// run-time libraries.
// It declares common functions and classes that are used in both runtimes.
// Implementation of some functions are provided in sanitizer_common, while
// others must be defined by run-time library itself.
//===----------------------------------------------------------------------===//
#ifndef SANITIZER_COMMON_H
#define SANITIZER_COMMON_H

#include "sanitizer_internal_defs.h"

namespace __sanitizer {
struct StackTrace;

// Constants.
const uptr kWordSize = SANITIZER_WORDSIZE / 8;
const uptr kWordSizeInBits = 8 * kWordSize;

#if defined(__powerpc__) || defined(__powerpc64__)
const uptr kCacheLineSize = 128;
#else
const uptr kCacheLineSize = 64;
#endif

extern const char *SanitizerToolName;  // Can be changed by the tool.

uptr GetPageSize();
uptr GetPageSizeCached();
uptr GetMmapGranularity();
// Threads
int GetPid();
uptr GetTid();
uptr GetThreadSelf();
void GetThreadStackTopAndBottom(bool at_initialization, uptr *stack_top,
                                uptr *stack_bottom);

// Memory management
void *MmapOrDie(uptr size, const char *mem_type);
void UnmapOrDie(void *addr, uptr size);
void *MmapFixedNoReserve(uptr fixed_addr, uptr size);
void *MmapFixedOrDie(uptr fixed_addr, uptr size);
void *Mprotect(uptr fixed_addr, uptr size);
// Map aligned chunk of address space; size and alignment are powers of two.
void *MmapAlignedOrDie(uptr size, uptr alignment, const char *mem_type);
// Used to check if we can map shadow memory to a fixed location.
bool MemoryRangeIsAvailable(uptr range_start, uptr range_end);
void FlushUnneededShadowMemory(uptr addr, uptr size);

// Internal allocator
void *InternalAlloc(uptr size);
void InternalFree(void *p);

// InternalScopedBuffer can be used instead of large stack arrays to
// keep frame size low.
// FIXME: use InternalAlloc instead of MmapOrDie once
// InternalAlloc is made libc-free.
template<typename T>
class InternalScopedBuffer {
 public:
  explicit InternalScopedBuffer(uptr cnt) {
    cnt_ = cnt;
    ptr_ = (T*)MmapOrDie(cnt * sizeof(T), "InternalScopedBuffer");
  }
  ~InternalScopedBuffer() {
    UnmapOrDie(ptr_, cnt_ * sizeof(T));
  }
  T &operator[](uptr i) { return ptr_[i]; }
  T *data() { return ptr_; }
  uptr size() { return cnt_ * sizeof(T); }

 private:
  T *ptr_;
  uptr cnt_;
  // Disallow evil constructors.
  InternalScopedBuffer(const InternalScopedBuffer&);
  void operator=(const InternalScopedBuffer&);
};

// Simple low-level (mmap-based) allocator for internal use. Doesn't have
// constructor, so all instances of LowLevelAllocator should be
// linker initialized.
class LowLevelAllocator {
 public:
  // Requires an external lock.
  void *Allocate(uptr size);
 private:
  char *allocated_end_;
  char *allocated_current_;
};
typedef void (*LowLevelAllocateCallback)(uptr ptr, uptr size);
// Allows to register tool-specific callbacks for LowLevelAllocator.
// Passing NULL removes the callback.
void SetLowLevelAllocateCallback(LowLevelAllocateCallback callback);

// IO
void RawWrite(const char *buffer);
bool PrintsToTty();
void Printf(const char *format, ...);
void Report(const char *format, ...);
void SetPrintfAndReportCallback(void (*callback)(const char *));

fd_t OpenFile(const char *filename, bool write);
// Opens the file 'file_name" and reads up to 'max_len' bytes.
// The resulting buffer is mmaped and stored in '*buff'.
// The size of the mmaped region is stored in '*buff_size',
// Returns the number of read bytes or 0 if file can not be opened.
uptr ReadFileToBuffer(const char *file_name, char **buff,
                      uptr *buff_size, uptr max_len);
// Maps given file to virtual memory, and returns pointer to it
// (or NULL if the mapping failes). Stores the size of mmaped region
// in '*buff_size'.
void *MapFileToMemory(const char *file_name, uptr *buff_size);

// OS
void DisableCoreDumper();
void DumpProcessMap();
bool FileExists(const char *filename);
const char *GetEnv(const char *name);
const char *GetPwd();
u32 GetUid();
void ReExec();
bool StackSizeIsUnlimited();
void SetStackSizeLimitInBytes(uptr limit);
void PrepareForSandboxing();

// Other
void SleepForSeconds(int seconds);
void SleepForMillis(int millis);
int Atexit(void (*function)(void));
void SortArray(uptr *array, uptr size);

// Exit
void NORETURN Abort();
void NORETURN Die();
void NORETURN SANITIZER_INTERFACE_ATTRIBUTE
CheckFailed(const char *file, int line, const char *cond, u64 v1, u64 v2);

// Set the name of the current thread to 'name', return true on succees.
// The name may be truncated to a system-dependent limit.
bool SanitizerSetThreadName(const char *name);
// Get the name of the current thread (no more than max_len bytes),
// return true on succees. name should have space for at least max_len+1 bytes.
bool SanitizerGetThreadName(char *name, int max_len);

// Specific tools may override behavior of "Die" and "CheckFailed" functions
// to do tool-specific job.
void SetDieCallback(void (*callback)(void));
typedef void (*CheckFailedCallbackType)(const char *, int, const char *,
                                       u64, u64);
void SetCheckFailedCallback(CheckFailedCallbackType callback);

// Construct a one-line string like
//  SanitizerToolName: error_type file:line function
// and call __sanitizer_report_error_summary on it.
void ReportErrorSummary(const char *error_type, const char *file,
                        int line, const char *function);

// Math
#if defined(_WIN32) && !defined(__clang__)
extern "C" {
unsigned char _BitScanForward(unsigned long *index, unsigned long mask);  // NOLINT
unsigned char _BitScanReverse(unsigned long *index, unsigned long mask);  // NOLINT
#if defined(_WIN64)
unsigned char _BitScanForward64(unsigned long *index, unsigned __int64 mask);  // NOLINT
unsigned char _BitScanReverse64(unsigned long *index, unsigned __int64 mask);  // NOLINT
#endif
}
#endif

INLINE uptr MostSignificantSetBitIndex(uptr x) {
  CHECK(x != 0);
  unsigned long up;  // NOLINT
#if !defined(_WIN32) || defined(__clang__)
  up = SANITIZER_WORDSIZE - 1 - __builtin_clzl(x);
#elif defined(_WIN64)
  _BitScanReverse64(&up, x);
#else
  _BitScanReverse(&up, x);
#endif
  return up;
}

INLINE bool IsPowerOfTwo(uptr x) {
  return (x & (x - 1)) == 0;
}

INLINE uptr RoundUpToPowerOfTwo(uptr size) {
  CHECK(size);
  if (IsPowerOfTwo(size)) return size;

  uptr up = MostSignificantSetBitIndex(size);
  CHECK(size < (1ULL << (up + 1)));
  CHECK(size > (1ULL << up));
  return 1UL << (up + 1);
}

INLINE uptr RoundUpTo(uptr size, uptr boundary) {
  CHECK(IsPowerOfTwo(boundary));
  return (size + boundary - 1) & ~(boundary - 1);
}

INLINE uptr RoundDownTo(uptr x, uptr boundary) {
  return x & ~(boundary - 1);
}

INLINE bool IsAligned(uptr a, uptr alignment) {
  return (a & (alignment - 1)) == 0;
}

INLINE uptr Log2(uptr x) {
  CHECK(IsPowerOfTwo(x));
#if !defined(_WIN32) || defined(__clang__)
  return __builtin_ctzl(x);
#elif defined(_WIN64)
  unsigned long ret;  // NOLINT
  _BitScanForward64(&ret, x);
  return ret;
#else
  unsigned long ret;  // NOLINT
  _BitScanForward(&ret, x);
  return ret;
#endif
}

// Don't use std::min, std::max or std::swap, to minimize dependency
// on libstdc++.
template<class T> T Min(T a, T b) { return a < b ? a : b; }
template<class T> T Max(T a, T b) { return a > b ? a : b; }
template<class T> void Swap(T& a, T& b) {
  T tmp = a;
  a = b;
  b = tmp;
}

// Char handling
INLINE bool IsSpace(int c) {
  return (c == ' ') || (c == '\n') || (c == '\t') ||
         (c == '\f') || (c == '\r') || (c == '\v');
}
INLINE bool IsDigit(int c) {
  return (c >= '0') && (c <= '9');
}
INLINE int ToLower(int c) {
  return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c;
}

#if SANITIZER_WORDSIZE == 64
# define FIRST_32_SECOND_64(a, b) (b)
#else
# define FIRST_32_SECOND_64(a, b) (a)
#endif

}  // namespace __sanitizer

#endif  // SANITIZER_COMMON_H
