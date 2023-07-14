//===-- sanitizer_getauxval.h -----------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Common getauxval() guards and definitions.
// getauxval() is not defined until glibc version 2.16, or until API level 21
// for Android.
// Implement the getauxval() compat function for NetBSD.
//
//===----------------------------------------------------------------------===//

#ifndef SANITIZER_GETAUXVAL_H
#define SANITIZER_GETAUXVAL_H

#include "sanitizer_platform.h"

#if SANITIZER_LINUX || SANITIZER_FUCHSIA

# include <features.h>

# ifndef __GLIBC_PREREQ
#  define __GLIBC_PREREQ(x, y) 0
# endif

# if __GLIBC_PREREQ(2, 16) || (SANITIZER_ANDROID && __ANDROID_API__ >= 21) || \
     SANITIZER_FUCHSIA
#  define SANITIZER_USE_GETAUXVAL 1
# else
#  define SANITIZER_USE_GETAUXVAL 0
# endif

# if SANITIZER_USE_GETAUXVAL
#  include <sys/auxv.h>
# else
// The weak getauxval definition allows to check for the function at runtime.
// This is useful for Android, when compiled at a lower API level yet running
// on a more recent platform that offers the function.
extern "C" SANITIZER_WEAK_ATTRIBUTE
unsigned long getauxval(unsigned long type);  // NOLINT
# endif

#elif SANITIZER_NETBSD

#define SANITIZER_USE_GETAUXVAL 1

#include <dlfcn.h>
#include <elf.h>

static inline decltype(AuxInfo::a_v) getauxval(decltype(AuxInfo::a_type) type) {
  for (const AuxInfo *aux = (const AuxInfo *)_dlauxinfo();
       aux->a_type != AT_NULL; ++aux) {
    if (type == aux->a_type)
      return aux->a_v;
  }

  return 0;
}

#endif

#endif // SANITIZER_GETAUXVAL_H
