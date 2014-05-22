//===-- asan_flags.h -------------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// ASan runtime flags.
//===----------------------------------------------------------------------===//

#ifndef ASAN_FLAGS_H
#define ASAN_FLAGS_H

#include "sanitizer_common/sanitizer_internal_defs.h"

// ASan flag values can be defined in four ways:
// 1) initialized with default values at startup.
// 2) overriden during compilation of ASan runtime by providing
//    compile definition ASAN_DEFAULT_OPTIONS.
// 3) overriden from string returned by user-specified function
//    __asan_default_options().
// 4) overriden from env variable ASAN_OPTIONS.

namespace __asan {

struct Flags {
  // Size (in bytes) of quarantine used to detect use-after-free errors.
  // Lower value may reduce memory usage but increase the chance of
  // false negatives.
  int  quarantine_size;
  // If set, uses in-process symbolizer from common sanitizer runtime.
  bool symbolize;
  // Verbosity level (0 - silent, 1 - a bit of output, 2+ - more output).
  int  verbosity;
  // Size (in bytes) of redzones around heap objects.
  // Requirement: redzone >= 32, is a power of two.
  int  redzone;
  // If set, prints some debugging information and does additional checks.
  bool debug;
  // Controls the way to handle globals (0 - don't detect buffer overflow
  // on globals, 1 - detect buffer overflow, 2 - print data about registered
  // globals).
  int  report_globals;
  // If set, attempts to catch initialization order issues.
  bool check_initialization_order;
  // Max number of stack frames kept for each allocation/deallocation.
  int  malloc_context_size;
  // If set, uses custom wrappers and replacements for libc string functions
  // to find more errors.
  bool replace_str;
  // If set, uses custom wrappers for memset/memcpy/memmove intinsics.
  bool replace_intrin;
  // Used on Mac only.
  bool mac_ignore_invalid_free;
  // ASan allocator flag. See asan_allocator.cc.
  bool use_fake_stack;
  // ASan allocator flag. Sets the maximal size of allocation request
  // that would return memory filled with zero bytes.
  int  max_malloc_fill_size;
  // Override exit status if something was reported.
  int  exitcode;
  // If set, user may manually mark memory regions as poisoned or unpoisoned.
  bool allow_user_poisoning;
  // Number of seconds to sleep between printing an error report and
  // terminating application. Useful for debug purposes (when one needs
  // to attach gdb, for example).
  int  sleep_before_dying;
  // If set, registers ASan custom segv handler.
  bool handle_segv;
  // If set, uses alternate stack for signal handling.
  bool use_sigaltstack;
  // Allow the users to work around the bug in Nvidia drivers prior to 295.*.
  bool check_malloc_usable_size;
  // If set, explicitly unmaps (huge) shadow at exit.
  bool unmap_shadow_on_exit;
  // If set, calls abort() instead of _exit() after printing an error report.
  bool abort_on_error;
  // Print various statistics after printing an error message or if atexit=1.
  bool print_stats;
  // Print the legend for the shadow bytes.
  bool print_legend;
  // If set, prints ASan exit stats even after program terminates successfully.
  bool atexit;
  // By default, disable core dumper on 64-bit - it makes little sense
  // to dump 16T+ core.
  bool disable_core;
  // Allow the tool to re-exec the program. This may interfere badly with the
  // debugger.
  bool allow_reexec;
  // Strips this prefix from file paths in error reports.
  const char *strip_path_prefix;
  // If set, prints not only thread creation stacks for threads in error report,
  // but also thread creation stacks for threads that created those threads,
  // etc. up to main thread.
  bool print_full_thread_history;
  // ASan will write logs to "log_path.pid" instead of stderr.
  const char *log_path;
  // Use fast (frame-pointer-based) unwinder on fatal errors (if available).
  bool fast_unwind_on_fatal;
  // Use fast (frame-pointer-based) unwinder on malloc/free (if available).
  bool fast_unwind_on_malloc;
  // Poison (or not) the heap memory on [de]allocation. Zero value is useful
  // for benchmarking the allocator or instrumentator.
  bool poison_heap;
  // Report errors on malloc/delete, new/free, new/delete[], etc.
  bool alloc_dealloc_mismatch;
  // Use stack depot instead of storing stacks in the redzones.
  bool use_stack_depot;
};

Flags *flags();
void InitializeFlags(Flags *f, const char *env);

}  // namespace __asan

#endif  // ASAN_FLAGS_H
