//===-- sanitizer_allocator_interface.h ------------------------- C++ -----===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Re-declaration of functions from public sanitizer allocator interface.
//
//===----------------------------------------------------------------------===//

#ifndef SANITIZER_ALLOCATOR_INTERFACE_H
#define SANITIZER_ALLOCATOR_INTERFACE_H

#include "sanitizer_internal_defs.h"

using __sanitizer::uptr;

extern "C" {
SANITIZER_INTERFACE_ATTRIBUTE
uptr __sanitizer_get_estimated_allocated_size(uptr size);
SANITIZER_INTERFACE_ATTRIBUTE int __sanitizer_get_ownership(const void *p);
SANITIZER_INTERFACE_ATTRIBUTE uptr
__sanitizer_get_allocated_size(const void *p);
SANITIZER_INTERFACE_ATTRIBUTE uptr __sanitizer_get_current_allocated_bytes();
SANITIZER_INTERFACE_ATTRIBUTE uptr __sanitizer_get_heap_size();
SANITIZER_INTERFACE_ATTRIBUTE uptr __sanitizer_get_free_bytes();
SANITIZER_INTERFACE_ATTRIBUTE uptr __sanitizer_get_unmapped_bytes();

SANITIZER_INTERFACE_ATTRIBUTE SANITIZER_WEAK_ATTRIBUTE
    /* OPTIONAL */ void __sanitizer_malloc_hook(void *ptr, uptr size);
SANITIZER_INTERFACE_ATTRIBUTE SANITIZER_WEAK_ATTRIBUTE
    /* OPTIONAL */ void __sanitizer_free_hook(void *ptr);
}  // extern "C"

#endif  // SANITIZER_ALLOCATOR_INTERFACE_H
