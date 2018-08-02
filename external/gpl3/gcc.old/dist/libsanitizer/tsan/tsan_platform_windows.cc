//===-- tsan_platform_windows.cc ------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// Windows-specific code.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_WINDOWS

#include "tsan_platform.h"

#include <stdlib.h>

namespace __tsan {

uptr GetShadowMemoryConsumption() {
  return 0;
}

void FlushShadowMemory() {
}

void WriteMemoryProfile(char *buf, uptr buf_size, uptr nthread, uptr nlive) {
}

void InitializePlatform() {
}

}  // namespace __tsan

#endif  // SANITIZER_WINDOWS
