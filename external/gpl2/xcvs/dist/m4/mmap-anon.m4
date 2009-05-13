# mmap-anon.m4 serial 3
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MMAP_ANON],
[
  dnl Work around a bug of AC_EGREP_CPP in autoconf-2.57.
  AC_REQUIRE([AC_PROG_CPP])
  AC_REQUIRE([AC_PROG_EGREP])

  dnl Persuade glibc <sys/mman.h> to define MAP_ANONYMOUS.
  AC_REQUIRE([AC_GNU_SOURCE])

  # Check for mmap()
  AC_FUNC_MMAP

  # Try to allow MAP_ANONYMOUS.
  gl_have_mmap_anonymous=no
  if test $ac_cv_func_mmap_fixed_mapped = yes; then
    AC_MSG_CHECKING([for MAP_ANONYMOUS])
    AC_EGREP_CPP([I cant identify this map.], [
#include <sys/mman.h>
#ifdef MAP_ANONYMOUS
    I cant identify this map.
#endif
],
      [gl_have_mmap_anonymous=yes])
    if test $gl_have_mmap_anonymous != yes; then
      AC_EGREP_CPP([I cant identify this map.], [
#include <sys/mman.h>
#ifdef MAP_ANON
    I cant identify this map.
#endif
],
        [AC_DEFINE(MAP_ANONYMOUS, MAP_ANON,
          [Define to a substitute value for mmap()'s MAP_ANONYMOUS flag.])
         gl_have_mmap_anonymous=yes])
    fi
    AC_MSG_RESULT($gl_have_mmap_anonymous)
    if test $gl_have_mmap_anonymous = yes; then
      AC_DEFINE(HAVE_MAP_ANONYMOUS, 1,
        [Define to 1 if mmap()'s MAP_ANONYMOUS flag is available after including
         config.h and <sys/mman.h>.])
    fi
  fi
])
