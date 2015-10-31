//===-- sanitizer_netbsd.cc -----------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is shared between AddressSanitizer and ThreadSanitizer
// run-time libraries and implements linux-specific functions from
// sanitizer_libc.h.
//===----------------------------------------------------------------------===//
#ifdef __NetBSD__

#include "sanitizer_common.h"
#include "sanitizer_internal_defs.h"
#include "sanitizer_libc.h"
#include "sanitizer_mutex.h"
#include "sanitizer_placement_new.h"
#include "sanitizer_procmaps.h"
#include "sanitizer_stacktrace.h"

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

namespace __sanitizer {

// --------------- sanitizer_libc.h
void *internal_mmap(void *addr, uptr length, int prot, int flags,
                    int fd, u64 offset) {
  return (void *)__syscall(SYS_mmap, addr, length, prot, flags,
  fd, 0, offset);
}

int internal_munmap(void *addr, uptr length) {
  return syscall(SYS_munmap, addr, length);
}

int internal_close(fd_t fd) {
  return syscall(SYS_close, fd);
}

fd_t internal_open(const char *filename, int flags) {
  return syscall(SYS_open, filename, flags);
}

fd_t internal_open(const char *filename, int flags, u32 mode) {
  return syscall(SYS_open, filename, flags, mode);
}

fd_t OpenFile(const char *filename, bool write) {
  return internal_open(filename,
      write ? O_WRONLY | O_CREAT /*| O_CLOEXEC*/ : O_RDONLY, 0660);
}

uptr internal_read(fd_t fd, void *buf, uptr count) {
  sptr res;
  HANDLE_EINTR(res, (sptr)syscall(SYS_read, fd, buf, count));
  return res;
}

uptr internal_write(fd_t fd, const void *buf, uptr count) {
  sptr res;
  HANDLE_EINTR(res, (sptr)syscall(SYS_write, fd, buf, count));
  return res;
}

int internal_stat(const char *path, void *buf) {
  return syscall(SYS___stat50, path, buf);
}

int internal_lstat(const char *path, void *buf) {
  return syscall(SYS___lstat50, path, buf);
}

int internal_fstat(fd_t fd, void *buf) {
  return syscall(SYS___fstat50, fd, buf);
}

uptr internal_filesize(fd_t fd) {
  struct stat st;
  if (internal_fstat(fd, &st))
    return -1;
  return (uptr)st.st_size;
}

int internal_dup2(int oldfd, int newfd) {
  return syscall(SYS_dup2, oldfd, newfd);
}

uptr internal_readlink(const char *path, char *buf, uptr bufsize) {
  return (uptr)syscall(SYS_readlink, path, buf, bufsize);
}

int internal_sched_yield() {
  return syscall(SYS_sched_yield);
}

void internal__exit(int exitcode) {
  syscall(SYS_exit, exitcode);
  Die();  // Unreachable.
}

// ----------------- sanitizer_common.h
bool FileExists(const char *filename) {
  struct stat st;
  if (syscall(SYS___stat50, filename, &st))
    return false;
  // Sanity check: filename is a regular file.
  return S_ISREG(st.st_mode);
}

uptr GetTid() {
  // XXX!
  return syscall(SYS_getpid);
}

void GetThreadStackTopAndBottom(bool at_initialization, uptr *stack_top,
                                uptr *stack_bottom) {
  static const uptr kMaxThreadStackSize = 256 * (1 << 20);  // 256M
  CHECK(stack_top);
  CHECK(stack_bottom);
  if (at_initialization) {
    // This is the main thread. Libpthread may not be initialized yet.
    struct rlimit rl;
    CHECK_EQ(getrlimit(RLIMIT_STACK, &rl), 0);

    // Find the mapping that contains a stack variable.
    MemoryMappingLayout proc_maps;
    uptr start, end, offset;
    uptr prev_end = 0;
    while (proc_maps.Next(&start, &end, &offset, 0, 0)) {
      if ((uptr)&rl < end)
        break;
      prev_end = end;
    }
    CHECK((uptr)&rl >= start && (uptr)&rl < end);

    // Get stacksize from rlimit, but clip it so that it does not overlap
    // with other mappings.
    uptr stacksize = rl.rlim_cur;
    if (stacksize > end - prev_end)
      stacksize = end - prev_end;
    // When running with unlimited stack size, we still want to set some limit.
    // The unlimited stack size is caused by 'ulimit -s unlimited'.
    // Also, for some reason, GNU make spawns subprocesses with unlimited stack.
    if (stacksize > kMaxThreadStackSize)
      stacksize = kMaxThreadStackSize;
    *stack_top = end;
    *stack_bottom = end - stacksize;
    return;
  }
  pthread_attr_t attr;
  CHECK_EQ(pthread_getattr_np(pthread_self(), &attr), 0);
  uptr stacksize = 0;
  void *stackaddr = 0;
  pthread_attr_getstack(&attr, &stackaddr, (size_t*)&stacksize);
  pthread_attr_destroy(&attr);

  *stack_top = (uptr)stackaddr + stacksize;
  *stack_bottom = (uptr)stackaddr;
  CHECK(stacksize < kMaxThreadStackSize);  // Sanity check.
}

// Like getenv, but reads env directly from /proc and does not use libc.
// This function should be called first inside __asan_init.
extern "C" char **environ;
const char *GetEnv(const char *name) {

  uptr namelen = internal_strlen(name);
  for (char **p = environ; *p; p++) {
    if (!internal_memcmp(*p, name, namelen) && (*p)[namelen] == '=')  // Match.
      return *p + namelen + 1;  // point after =
  }
  return 0;  // Not found.
}

#ifdef __GLIBC__

extern "C" {
  extern void *__libc_stack_end;
}

static void GetArgsAndEnv(char ***argv, char ***envp) {
  uptr *stack_end = (uptr *)__libc_stack_end;
  int argc = *stack_end;
  *argv = (char**)(stack_end + 1);
  *envp = (char**)(stack_end + argc + 2);
}

#else  // __GLIBC__

static void ReadNullSepFileToArray(const char *path, char ***arr,
                                   int arr_size) {
  char *buff;
  uptr buff_size = 0;
  *arr = (char **)MmapOrDie(arr_size * sizeof(char *), "NullSepFileArray");
  ReadFileToBuffer(path, &buff, &buff_size, 1024 * 1024);
  (*arr)[0] = buff;
  int count, i;
  for (count = 1, i = 1; ; i++) {
    if (buff[i] == 0) {
      if (buff[i+1] == 0) break;
      (*arr)[count] = &buff[i+1];
      CHECK_LE(count, arr_size - 1);  // FIXME: make this more flexible.
      count++;
    }
  }
  (*arr)[count] = 0;
}

static void GetArgsAndEnv(char ***argv, char ***envp) {
  static const int kMaxArgv = 2000, kMaxEnvp = 2000;
  ReadNullSepFileToArray("/proc/self/cmdline", argv, kMaxArgv);
  ReadNullSepFileToArray("/proc/self/environ", envp, kMaxEnvp);
}

#endif  // __GLIBC__

void ReExec() {
  char **argv, **envp;
  GetArgsAndEnv(&argv, &envp);
  execve("/proc/self/exe", argv, envp);
  Printf("execve failed, errno %d\n", errno);
  Die();
}

void PrepareForSandboxing() {
  // Some kinds of sandboxes may forbid filesystem access, so we won't be able
  // to read the file mappings from /proc/self/maps. Luckily, neither the
  // process will be able to load additional libraries, so it's fine to use the
  // cached mappings.
  MemoryMappingLayout::CacheMemoryMappings();
}

// ----------------- sanitizer_procmaps.h
// Linker initialized.
ProcSelfMapsBuff MemoryMappingLayout::cached_proc_self_maps_;
StaticSpinMutex MemoryMappingLayout::cache_lock_;  // Linker initialized.

MemoryMappingLayout::MemoryMappingLayout() {
  proc_self_maps_.len =
      ReadFileToBuffer("/proc/self/maps", &proc_self_maps_.data,
                       &proc_self_maps_.mmaped_size, 1 << 26);
  if (proc_self_maps_.mmaped_size == 0) {
    LoadFromCache();
    CHECK_GT(proc_self_maps_.len, 0);
  }
  // internal_write(2, proc_self_maps_.data, proc_self_maps_.len);
  Reset();
  // FIXME: in the future we may want to cache the mappings on demand only.
  CacheMemoryMappings();
}

MemoryMappingLayout::~MemoryMappingLayout() {
  // Only unmap the buffer if it is different from the cached one. Otherwise
  // it will be unmapped when the cache is refreshed.
  if (proc_self_maps_.data != cached_proc_self_maps_.data) {
    UnmapOrDie(proc_self_maps_.data, proc_self_maps_.mmaped_size);
  }
}

void MemoryMappingLayout::Reset() {
  current_ = proc_self_maps_.data;
}

// static
void MemoryMappingLayout::CacheMemoryMappings() {
  SpinMutexLock l(&cache_lock_);
  // Don't invalidate the cache if the mappings are unavailable.
  ProcSelfMapsBuff old_proc_self_maps;
  old_proc_self_maps = cached_proc_self_maps_;
  cached_proc_self_maps_.len =
      ReadFileToBuffer("/proc/self/maps", &cached_proc_self_maps_.data,
                       &cached_proc_self_maps_.mmaped_size, 1 << 26);
  if (cached_proc_self_maps_.mmaped_size == 0) {
    cached_proc_self_maps_ = old_proc_self_maps;
  } else {
    if (old_proc_self_maps.mmaped_size) {
      UnmapOrDie(old_proc_self_maps.data,
                 old_proc_self_maps.mmaped_size);
    }
  }
}

void MemoryMappingLayout::LoadFromCache() {
  SpinMutexLock l(&cache_lock_);
  if (cached_proc_self_maps_.data) {
    proc_self_maps_ = cached_proc_self_maps_;
  }
}

// Parse a hex value in str and update str.
static uptr ParseHex(char **str) {
  uptr x = 0;
  char *s;
  for (s = *str; ; s++) {
    char c = *s;
    uptr v = 0;
    if (c >= '0' && c <= '9')
      v = c - '0';
    else if (c >= 'a' && c <= 'f')
      v = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      v = c - 'A' + 10;
    else
      break;
    x = x * 16 + v;
  }
  *str = s;
  return x;
}

static bool IsOnOf(char c, char c1, char c2) {
  return c == c1 || c == c2;
}

static bool IsDecimal(char c) {
  return c >= '0' && c <= '9';
}

bool MemoryMappingLayout::Next(uptr *start, uptr *end, uptr *offset,
                               char filename[], uptr filename_size) {
  char *last = proc_self_maps_.data + proc_self_maps_.len;
  if (current_ >= last) return false;
  uptr dummy;
  if (!start) start = &dummy;
  if (!end) end = &dummy;
  if (!offset) offset = &dummy;
  char *next_line = (char*)internal_memchr(current_, '\n', last - current_);
  if (next_line == 0)
    next_line = last;
  // Example: 08048000-08056000 r-xp 00000000 03:0c 64593   /foo/bar
  *start = ParseHex(&current_);
  CHECK_EQ(*current_++, '-');
  *end = ParseHex(&current_);
  CHECK_EQ(*current_++, ' ');
  CHECK(IsOnOf(*current_++, '-', 'r'));
  CHECK(IsOnOf(*current_++, '-', 'w'));
  CHECK(IsOnOf(*current_++, '-', 'x'));
  CHECK(IsOnOf(*current_++, 's', 'p'));
  CHECK_EQ(*current_++, ' ');
  *offset = ParseHex(&current_);
  CHECK_EQ(*current_++, ' ');
  ParseHex(&current_);
  CHECK_EQ(*current_++, ':');
  ParseHex(&current_);
  CHECK_EQ(*current_++, ' ');
  while (IsDecimal(*current_))
    current_++;
  CHECK_EQ(*current_++, ' ');
  // Skip spaces.
  while (current_ < next_line && *current_ == ' ')
    current_++;
  // Fill in the filename.
  uptr i = 0;
  while (current_ < next_line) {
    if (filename && i < filename_size - 1)
      filename[i++] = *current_;
    current_++;
  }
  if (filename && i < filename_size)
    filename[i] = 0;
  current_ = next_line + 1;
  return true;
}

// Gets the object name and the offset by walking MemoryMappingLayout.
bool MemoryMappingLayout::GetObjectNameAndOffset(uptr addr, uptr *offset,
                                                 char filename[],
                                                 uptr filename_size) {
  return IterateForObjectNameAndOffset(addr, offset, filename, filename_size);
}

bool SanitizerSetThreadName(const char *name) {
   return 0 == pthread_setname_np(pthread_self(), "%s", (void *)(intptr_t)name);
}

bool SanitizerGetThreadName(char *name, int max_len) {
   return 0 == pthread_getname_np(pthread_self(), name, max_len);
}

#ifndef SANITIZER_GO
//------------------------- SlowUnwindStack -----------------------------------
#if defined(__arm__) && defined(__ARM_EABI__) && !defined(__ARM_DWARF_EH__)
#include "unwind-arm-common.h"
#define UNWIND_STOP _URC_END_OF_STACK
#define UNWIND_CONTINUE _URC_NO_REASON
#else
#include <unwind.h>
#define UNWIND_STOP _URC_NORMAL_STOP
#define UNWIND_CONTINUE _URC_NO_REASON
#endif

uptr Unwind_GetIP(struct _Unwind_Context *ctx) {
#if defined(__arm__) && defined(__ARM_EABI__) && !defined(__ARM_DWARF_EH__)
  uptr val;
  _Unwind_VRS_Result res = _Unwind_VRS_Get(ctx, _UVRSC_CORE,
      15 /* r15 = PC */, _UVRSD_UINT32, &val);
  CHECK(res == _UVRSR_OK && "_Unwind_VRS_Get failed");
  // Clear the Thumb bit.
  return val & ~(uptr)1;
#else
  return (uptr)_Unwind_GetIP(ctx);
#endif
}

_Unwind_Reason_Code Unwind_Trace(struct _Unwind_Context *ctx, void *param) {
  StackTrace *b = (StackTrace*)param;
  CHECK(b->size < b->max_size);
  uptr pc = Unwind_GetIP(ctx);
  b->trace[b->size++] = pc;
  if (b->size == b->max_size) return UNWIND_STOP;
  return UNWIND_CONTINUE;
}

static bool MatchPc(uptr cur_pc, uptr trace_pc) {
  return cur_pc - trace_pc <= 64 || trace_pc - cur_pc <= 64;
}

void StackTrace::SlowUnwindStack(uptr pc, uptr max_depth) {
  this->size = 0;
  this->max_size = max_depth;
  if (max_depth > 1) {
    _Unwind_Backtrace(Unwind_Trace, this);
    // We need to pop a few frames so that pc is on top.
    // trace[0] belongs to the current function so we always pop it.
    int to_pop = 1;
    /**/ if (size > 1 && MatchPc(pc, trace[1])) to_pop = 1;
    else if (size > 2 && MatchPc(pc, trace[2])) to_pop = 2;
    else if (size > 3 && MatchPc(pc, trace[3])) to_pop = 3;
    else if (size > 4 && MatchPc(pc, trace[4])) to_pop = 4;
    else if (size > 5 && MatchPc(pc, trace[5])) to_pop = 5;
    this->PopStackFrames(to_pop);
  }
  this->trace[0] = pc;
}

#endif  // #ifndef SANITIZER_GO

enum MutexState {
  MtxUnlocked = 0,
  MtxLocked = 1,
  MtxSleeping = 2
};

BlockingMutex::BlockingMutex(LinkerInitialized) {
  CHECK_EQ(owner_, 0);
}

void BlockingMutex::Lock() {
  atomic_uint32_t *m = reinterpret_cast<atomic_uint32_t *>(&opaque_storage_);
  if (atomic_exchange(m, MtxLocked, memory_order_acquire) == MtxUnlocked)
    return;
  while (atomic_exchange(m, MtxSleeping, memory_order_acquire) != MtxUnlocked)
	syscall(SYS_sched_yield);
}

void BlockingMutex::Unlock() {
  atomic_uint32_t *m = reinterpret_cast<atomic_uint32_t *>(&opaque_storage_);
  u32 v = atomic_exchange(m, MtxUnlocked, memory_order_relaxed);
  CHECK_NE(v, MtxUnlocked);
#if 0
  if (v == MtxSleeping)
    syscall(__NR_futex, m, FUTEX_WAKE, 1, 0, 0, 0);
#endif
}

}  // namespace __sanitizer

#endif  // __NetBSD__
