//===-- asan_allocator.h ----------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// ASan-private header for asan_allocator2.cc.
//===----------------------------------------------------------------------===//

#ifndef ASAN_ALLOCATOR_H
#define ASAN_ALLOCATOR_H

#include "asan_internal.h"
#include "asan_interceptors.h"
#include "sanitizer_common/sanitizer_allocator.h"
#include "sanitizer_common/sanitizer_list.h"

namespace __asan {

enum AllocType {
  FROM_MALLOC = 1,  // Memory block came from malloc, calloc, realloc, etc.
  FROM_NEW = 2,     // Memory block came from operator new.
  FROM_NEW_BR = 3   // Memory block came from operator new [ ]
};

static const uptr kNumberOfSizeClasses = 255;
struct AsanChunk;

void InitializeAllocator();
void ReInitializeAllocator();

class AsanChunkView {
 public:
  explicit AsanChunkView(AsanChunk *chunk) : chunk_(chunk) {}
  bool IsValid();   // Checks if AsanChunkView points to a valid allocated
                    // or quarantined chunk.
  uptr Beg();       // First byte of user memory.
  uptr End();       // Last byte of user memory.
  uptr UsedSize();  // Size requested by the user.
  uptr AllocTid();
  uptr FreeTid();
  bool Eq(const AsanChunkView &c) const { return chunk_ == c.chunk_; }
  StackTrace GetAllocStack();
  StackTrace GetFreeStack();
  bool AddrIsInside(uptr addr, uptr access_size, sptr *offset) {
    if (addr >= Beg() && (addr + access_size) <= End()) {
      *offset = addr - Beg();
      return true;
    }
    return false;
  }
  bool AddrIsAtLeft(uptr addr, uptr access_size, sptr *offset) {
    (void)access_size;
    if (addr < Beg()) {
      *offset = Beg() - addr;
      return true;
    }
    return false;
  }
  bool AddrIsAtRight(uptr addr, uptr access_size, sptr *offset) {
    if (addr + access_size > End()) {
      *offset = addr - End();
      return true;
    }
    return false;
  }

 private:
  AsanChunk *const chunk_;
};

AsanChunkView FindHeapChunkByAddress(uptr address);

// List of AsanChunks with total size.
class AsanChunkFifoList: public IntrusiveList<AsanChunk> {
 public:
  explicit AsanChunkFifoList(LinkerInitialized) { }
  AsanChunkFifoList() { clear(); }
  void Push(AsanChunk *n);
  void PushList(AsanChunkFifoList *q);
  AsanChunk *Pop();
  uptr size() { return size_; }
  void clear() {
    IntrusiveList<AsanChunk>::clear();
    size_ = 0;
  }
 private:
  uptr size_;
};

struct AsanMapUnmapCallback {
  void OnMap(uptr p, uptr size) const;
  void OnUnmap(uptr p, uptr size) const;
};

#if SANITIZER_CAN_USE_ALLOCATOR64
# if defined(__powerpc64__)
const uptr kAllocatorSpace =  0xa0000000000ULL;
const uptr kAllocatorSize  =  0x20000000000ULL;  // 2T.
# else
const uptr kAllocatorSpace = 0x600000000000ULL;
const uptr kAllocatorSize  =  0x40000000000ULL;  // 4T.
# endif
typedef DefaultSizeClassMap SizeClassMap;
typedef SizeClassAllocator64<kAllocatorSpace, kAllocatorSize, 0 /*metadata*/,
    SizeClassMap, AsanMapUnmapCallback> PrimaryAllocator;
#else  // Fallback to SizeClassAllocator32.
static const uptr kRegionSizeLog = 20;
static const uptr kNumRegions = SANITIZER_MMAP_RANGE_SIZE >> kRegionSizeLog;
# if SANITIZER_WORDSIZE == 32
typedef FlatByteMap<kNumRegions> ByteMap;
# elif SANITIZER_WORDSIZE == 64
typedef TwoLevelByteMap<(kNumRegions >> 12), 1 << 12> ByteMap;
# endif
typedef CompactSizeClassMap SizeClassMap;
typedef SizeClassAllocator32<0, SANITIZER_MMAP_RANGE_SIZE, 16,
  SizeClassMap, kRegionSizeLog,
  ByteMap,
  AsanMapUnmapCallback> PrimaryAllocator;
#endif  // SANITIZER_CAN_USE_ALLOCATOR64

typedef SizeClassAllocatorLocalCache<PrimaryAllocator> AllocatorCache;
typedef LargeMmapAllocator<AsanMapUnmapCallback> SecondaryAllocator;
typedef CombinedAllocator<PrimaryAllocator, AllocatorCache,
    SecondaryAllocator> Allocator;


struct AsanThreadLocalMallocStorage {
  uptr quarantine_cache[16];
  AllocatorCache allocator2_cache;
  void CommitBack();
 private:
  // These objects are allocated via mmap() and are zero-initialized.
  AsanThreadLocalMallocStorage() {}
};

void *asan_memalign(uptr alignment, uptr size, BufferedStackTrace *stack,
                    AllocType alloc_type);
void asan_free(void *ptr, BufferedStackTrace *stack, AllocType alloc_type);
void asan_sized_free(void *ptr, uptr size, BufferedStackTrace *stack,
                     AllocType alloc_type);

void *asan_malloc(uptr size, BufferedStackTrace *stack);
void *asan_calloc(uptr nmemb, uptr size, BufferedStackTrace *stack);
void *asan_realloc(void *p, uptr size, BufferedStackTrace *stack);
void *asan_valloc(uptr size, BufferedStackTrace *stack);
void *asan_pvalloc(uptr size, BufferedStackTrace *stack);

int asan_posix_memalign(void **memptr, uptr alignment, uptr size,
                        BufferedStackTrace *stack);
uptr asan_malloc_usable_size(void *ptr, uptr pc, uptr bp);

uptr asan_mz_size(const void *ptr);
void asan_mz_force_lock();
void asan_mz_force_unlock();

void PrintInternalAllocatorStats();

}  // namespace __asan
#endif  // ASAN_ALLOCATOR_H
