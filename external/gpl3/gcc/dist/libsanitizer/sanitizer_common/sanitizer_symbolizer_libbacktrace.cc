//===-- sanitizer_symbolizer_libbacktrace.cc ------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is shared between AddressSanitizer and ThreadSanitizer
// run-time libraries.
// Libbacktrace implementation of symbolizer parts.
//===----------------------------------------------------------------------===//

#include "sanitizer_platform.h"

#include "sanitizer_internal_defs.h"
#include "sanitizer_symbolizer.h"
#include "sanitizer_symbolizer_libbacktrace.h"

#if SANITIZER_LIBBACKTRACE
# include "backtrace-supported.h"
# if SANITIZER_POSIX && BACKTRACE_SUPPORTED && !BACKTRACE_USES_MALLOC
#  include "backtrace.h"
#  if SANITIZER_CP_DEMANGLE
#   undef ARRAY_SIZE
#   include "demangle.h"
#  endif
# else
#  define SANITIZER_LIBBACKTRACE 0
# endif
#endif

namespace __sanitizer {

#if SANITIZER_LIBBACKTRACE

namespace {

# if SANITIZER_CP_DEMANGLE
struct CplusV3DemangleData {
  char *buf;
  uptr size, allocated;
};

extern "C" {
static void CplusV3DemangleCallback(const char *s, size_t l, void *vdata) {
  CplusV3DemangleData *data = (CplusV3DemangleData *)vdata;
  uptr needed = data->size + l + 1;
  if (needed > data->allocated) {
    data->allocated *= 2;
    if (needed > data->allocated)
      data->allocated = needed;
    char *buf = (char *)InternalAlloc(data->allocated);
    if (data->buf) {
      internal_memcpy(buf, data->buf, data->size);
      InternalFree(data->buf);
    }
    data->buf = buf;
  }
  internal_memcpy(data->buf + data->size, s, l);
  data->buf[data->size + l] = '\0';
  data->size += l;
}
}  // extern "C"

char *CplusV3Demangle(const char *name) {
  CplusV3DemangleData data;
  data.buf = 0;
  data.size = 0;
  data.allocated = 0;
  if (cplus_demangle_v3_callback(name, DMGL_PARAMS | DMGL_ANSI,
                                 CplusV3DemangleCallback, &data)) {
    if (data.size + 64 > data.allocated)
      return data.buf;
    char *buf = internal_strdup(data.buf);
    InternalFree(data.buf);
    return buf;
  }
  if (data.buf)
    InternalFree(data.buf);
  return 0;
}
# endif  // SANITIZER_CP_DEMANGLE

struct SymbolizeCodeData {
  AddressInfo *frames;
  uptr n_frames;
  uptr max_frames;
  const char *module_name;
  uptr module_offset;
};

extern "C" {
static int SymbolizeCodePCInfoCallback(void *vdata, uintptr_t addr,
                                       const char *filename, int lineno,
                                       const char *function) {
  SymbolizeCodeData *cdata = (SymbolizeCodeData *)vdata;
  if (function) {
    AddressInfo *info = &cdata->frames[cdata->n_frames++];
    info->Clear();
    info->FillAddressAndModuleInfo(addr, cdata->module_name,
                                   cdata->module_offset);
    info->function = LibbacktraceSymbolizer::Demangle(function, true);
    if (filename)
      info->file = internal_strdup(filename);
    info->line = lineno;
    if (cdata->n_frames == cdata->max_frames)
      return 1;
  }
  return 0;
}

static void SymbolizeCodeCallback(void *vdata, uintptr_t addr,
                                  const char *symname, uintptr_t, uintptr_t) {
  SymbolizeCodeData *cdata = (SymbolizeCodeData *)vdata;
  if (symname) {
    AddressInfo *info = &cdata->frames[0];
    info->Clear();
    info->FillAddressAndModuleInfo(addr, cdata->module_name,
                                   cdata->module_offset);
    info->function = LibbacktraceSymbolizer::Demangle(symname, true);
    cdata->n_frames = 1;
  }
}

static void SymbolizeDataCallback(void *vdata, uintptr_t, const char *symname,
                                  uintptr_t symval, uintptr_t symsize) {
  DataInfo *info = (DataInfo *)vdata;
  if (symname && symval) {
    info->name = LibbacktraceSymbolizer::Demangle(symname, true);
    info->start = symval;
    info->size = symsize;
  }
}

static void ErrorCallback(void *, const char *, int) {}
}  // extern "C"

}  // namespace

LibbacktraceSymbolizer *LibbacktraceSymbolizer::get(LowLevelAllocator *alloc) {
  // State created in backtrace_create_state is leaked.
  void *state = (void *)(backtrace_create_state("/proc/self/exe", 0,
                                                ErrorCallback, NULL));
  if (!state)
    return 0;
  return new(*alloc) LibbacktraceSymbolizer(state);
}

uptr LibbacktraceSymbolizer::SymbolizeCode(uptr addr, AddressInfo *frames,
                                           uptr max_frames,
                                           const char *module_name,
                                           uptr module_offset) {
  SymbolizeCodeData data;
  data.frames = frames;
  data.n_frames = 0;
  data.max_frames = max_frames;
  data.module_name = module_name;
  data.module_offset = module_offset;
  backtrace_pcinfo((backtrace_state *)state_, addr, SymbolizeCodePCInfoCallback,
                   ErrorCallback, &data);
  if (data.n_frames)
    return data.n_frames;
  backtrace_syminfo((backtrace_state *)state_, addr, SymbolizeCodeCallback,
                    ErrorCallback, &data);
  return data.n_frames;
}

bool LibbacktraceSymbolizer::SymbolizeData(uptr addr, DataInfo *info) {
  backtrace_syminfo((backtrace_state *)state_, addr, SymbolizeDataCallback,
                    ErrorCallback, info);
  return true;
}

#else  // SANITIZER_LIBBACKTRACE

LibbacktraceSymbolizer *LibbacktraceSymbolizer::get(LowLevelAllocator *alloc) {
  return 0;
}

uptr LibbacktraceSymbolizer::SymbolizeCode(uptr addr, AddressInfo *frames,
                                           uptr max_frames,
                                           const char *module_name,
                                           uptr module_offset) {
  (void)state_;
  return 0;
}

bool LibbacktraceSymbolizer::SymbolizeData(uptr addr, DataInfo *info) {
  return false;
}

#endif  // SANITIZER_LIBBACKTRACE

char *LibbacktraceSymbolizer::Demangle(const char *name, bool always_alloc) {
#if SANITIZER_LIBBACKTRACE && SANITIZER_CP_DEMANGLE
  if (char *demangled = CplusV3Demangle(name))
    return demangled;
#endif
  if (always_alloc)
    return internal_strdup(name);
  return 0;
}

}  // namespace __sanitizer
