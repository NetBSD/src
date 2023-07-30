//===-- sanitizer_fuchsia.h ------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// Fuchsia-specific sanitizer support.
//
//===---------------------------------------------------------------------===//
#ifndef SANITIZER_FUCHSIA_H
#define SANITIZER_FUCHSIA_H

#include "sanitizer_platform.h"
#if SANITIZER_FUCHSIA

#include "sanitizer_common.h"

#include <zircon/sanitizer.h>
#include <zircon/syscalls/object.h>

namespace __sanitizer {

extern uptr MainThreadStackBase, MainThreadStackSize;
extern sanitizer_shadow_bounds_t ShadowBounds;

struct MemoryMappingLayoutData {
  InternalMmapVector<zx_info_maps_t> data;
  size_t current;  // Current index into the vector.
};

void InitShadowBounds();

}  // namespace __sanitizer

#endif  // SANITIZER_FUCHSIA
#endif  // SANITIZER_FUCHSIA_H
