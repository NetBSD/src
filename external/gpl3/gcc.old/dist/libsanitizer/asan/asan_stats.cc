//===-- asan_stats.cc -----------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// Code related to statistics collected by AddressSanitizer.
//===----------------------------------------------------------------------===//
#include "asan_interceptors.h"
#include "asan_internal.h"
#include "asan_stats.h"
#include "asan_thread_registry.h"
#include "sanitizer_common/sanitizer_stackdepot.h"

namespace __asan {

AsanStats::AsanStats() {
  CHECK(REAL(memset) != 0);
  REAL(memset)(this, 0, sizeof(AsanStats));
}

static void PrintMallocStatsArray(const char *prefix,
                                  uptr (&array)[kNumberOfSizeClasses]) {
  Printf("%s", prefix);
  for (uptr i = 0; i < kNumberOfSizeClasses; i++) {
    if (!array[i]) continue;
    Printf("%zu:%zu; ", i, array[i]);
  }
  Printf("\n");
}

void AsanStats::Print() {
  Printf("Stats: %zuM malloced (%zuM for red zones) by %zu calls\n",
             malloced>>20, malloced_redzones>>20, mallocs);
  Printf("Stats: %zuM realloced by %zu calls\n", realloced>>20, reallocs);
  Printf("Stats: %zuM freed by %zu calls\n", freed>>20, frees);
  Printf("Stats: %zuM really freed by %zu calls\n",
             really_freed>>20, real_frees);
  Printf("Stats: %zuM (%zuM-%zuM) mmaped; %zu maps, %zu unmaps\n",
             (mmaped-munmaped)>>20, mmaped>>20, munmaped>>20,
             mmaps, munmaps);

  PrintMallocStatsArray("  mmaps   by size class: ", mmaped_by_size);
  PrintMallocStatsArray("  mallocs by size class: ", malloced_by_size);
  PrintMallocStatsArray("  frees   by size class: ", freed_by_size);
  PrintMallocStatsArray("  rfrees  by size class: ", really_freed_by_size);
  Printf("Stats: malloc large: %zu small slow: %zu\n",
             malloc_large, malloc_small_slow);
}

static BlockingMutex print_lock(LINKER_INITIALIZED);

static void PrintAccumulatedStats() {
  AsanStats stats;
  asanThreadRegistry().GetAccumulatedStats(&stats);
  // Use lock to keep reports from mixing up.
  BlockingMutexLock lock(&print_lock);
  stats.Print();
  StackDepotStats *stack_depot_stats = StackDepotGetStats();
  Printf("Stats: StackDepot: %zd ids; %zdM mapped\n",
         stack_depot_stats->n_uniq_ids, stack_depot_stats->mapped >> 20);
  PrintInternalAllocatorStats();
}

}  // namespace __asan

// ---------------------- Interface ---------------- {{{1
using namespace __asan;  // NOLINT

uptr __asan_get_current_allocated_bytes() {
  return asanThreadRegistry().GetCurrentAllocatedBytes();
}

uptr __asan_get_heap_size() {
  return asanThreadRegistry().GetHeapSize();
}

uptr __asan_get_free_bytes() {
  return asanThreadRegistry().GetFreeBytes();
}

uptr __asan_get_unmapped_bytes() {
  return 0;
}

void __asan_print_accumulated_stats() {
  PrintAccumulatedStats();
}
