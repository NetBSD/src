//===-- sanitizer_coverage_mapping.cc -------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Mmap-based implementation of sanitizer coverage.
//
// This is part of the implementation of code coverage that does not require
// __sanitizer_cov_dump() call. Data is stored in 2 files per process.
//
// $pid.sancov.map describes process memory layout in the following text-based
// format:
// <pointer size in bits>  // 1 line, 32 or 64
// <mapping start> <mapping end> <base address> <dso name> // repeated
// ...
// Mapping lines are NOT sorted. This file is updated every time memory layout
// is changed (i.e. in dlopen() and dlclose() interceptors).
//
// $pid.sancov.raw is a binary dump of PC values, sizeof(uptr) each. Again, not
// sorted. This file is extended by 64Kb at a time and mapped into memory. It
// contains one or more 0 words at the end, up to the next 64Kb aligned offset.
//
// To convert these 2 files to the usual .sancov format, run sancov.py rawunpack
// $pid.sancov.raw.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_allocator_internal.h"
#include "sanitizer_libc.h"
#include "sanitizer_procmaps.h"

namespace __sanitizer {

static const uptr kMaxNumberOfModules = 1 << 14;
static const uptr kMaxTextSize = 64 * 1024;

struct CachedMapping {
 public:
  bool NeedsUpdate(uptr pc) {
    int new_pid = internal_getpid();
    if (last_pid == new_pid && pc && pc >= last_range_start &&
        pc < last_range_end)
      return false;
    last_pid = new_pid;
    return true;
  }

  void SetModuleRange(uptr start, uptr end) {
    last_range_start = start;
    last_range_end = end;
  }

 private:
  uptr last_range_start, last_range_end;
  int last_pid;
};

static CachedMapping cached_mapping;
static StaticSpinMutex mapping_mu;

void CovUpdateMapping(uptr caller_pc) {
  if (!common_flags()->coverage || !common_flags()->coverage_direct) return;

  SpinMutexLock l(&mapping_mu);

  if (!cached_mapping.NeedsUpdate(caller_pc))
    return;

  InternalScopedString text(kMaxTextSize);
  InternalScopedBuffer<char> modules_data(kMaxNumberOfModules *
                                          sizeof(LoadedModule));
  LoadedModule *modules = (LoadedModule *)modules_data.data();
  CHECK(modules);
  int n_modules = GetListOfModules(modules, kMaxNumberOfModules,
                                   /* filter */ 0);

  text.append("%d\n", sizeof(uptr) * 8);
  for (int i = 0; i < n_modules; ++i) {
    const char *module_name = StripModuleName(modules[i].full_name());
    for (unsigned j = 0; j < modules[i].n_ranges(); ++j) {
      if (modules[i].address_range_executable(j)) {
        uptr start = modules[i].address_range_start(j);
        uptr end = modules[i].address_range_end(j);
        uptr base = modules[i].base_address();
        text.append("%zx %zx %zx %s\n", start, end, base, module_name);
        if (caller_pc && caller_pc >= start && caller_pc < end)
          cached_mapping.SetModuleRange(start, end);
      }
    }
  }

  int err;
  InternalScopedString tmp_path(64 +
                                internal_strlen(common_flags()->coverage_dir));
  uptr res = internal_snprintf((char *)tmp_path.data(), tmp_path.size(),
                    "%s/%zd.sancov.map.tmp", common_flags()->coverage_dir,
                    internal_getpid());
  CHECK_LE(res, tmp_path.size());
  uptr map_fd = OpenFile(tmp_path.data(), true);
  if (internal_iserror(map_fd)) {
    Report(" Coverage: failed to open %s for writing\n", tmp_path.data());
    Die();
  }

  res = internal_write(map_fd, text.data(), text.length());
  if (internal_iserror(res, &err)) {
    Printf("sancov.map write failed: %d\n", err);
    Die();
  }
  internal_close(map_fd);

  InternalScopedString path(64 + internal_strlen(common_flags()->coverage_dir));
  res = internal_snprintf((char *)path.data(), path.size(), "%s/%zd.sancov.map",
                    common_flags()->coverage_dir, internal_getpid());
  CHECK_LE(res, path.size());
  res = internal_rename(tmp_path.data(), path.data());
  if (internal_iserror(res, &err)) {
    Printf("sancov.map rename failed: %d\n", err);
    Die();
  }
}

}  // namespace __sanitizer
