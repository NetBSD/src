# memmem.m4 serial 14
dnl Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009, 2010 Free Software
dnl Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Check that memmem is present.
AC_DEFUN([gl_FUNC_MEMMEM_SIMPLE],
[
  dnl Persuade glibc <string.h> to declare memmem().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_REPLACE_FUNCS([memmem])
  AC_CHECK_DECLS_ONCE([memmem])
  if test $ac_cv_have_decl_memmem = no; then
    HAVE_DECL_MEMMEM=0
  fi
  gl_PREREQ_MEMMEM
]) # gl_FUNC_MEMMEM_SIMPLE

dnl Additionally, check that memmem is efficient and handles empty needles.
AC_DEFUN([gl_FUNC_MEMMEM],
[
  AC_REQUIRE([gl_FUNC_MEMMEM_SIMPLE])
  if test $ac_cv_have_decl_memmem = yes; then
    AC_CACHE_CHECK([whether memmem works in linear time],
      [gl_cv_func_memmem_works],
      [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <signal.h> /* for signal */
#include <string.h> /* for memmem */
#include <stdlib.h> /* for malloc */
#include <unistd.h> /* for alarm */
]], [[size_t m = 1000000;
    char *haystack = (char *) malloc (2 * m + 1);
    char *needle = (char *) malloc (m + 1);
    void *result = 0;
    /* Failure to compile this test due to missing alarm is okay,
       since all such platforms (mingw) also lack memmem.  */
    signal (SIGALRM, SIG_DFL);
    alarm (5);
    /* Check for quadratic performance.  */
    if (haystack && needle)
      {
        memset (haystack, 'A', 2 * m);
        haystack[2 * m] = 'B';
        memset (needle, 'A', m);
        needle[m] = 'B';
        result = memmem (haystack, 2 * m + 1, needle, m + 1);
      }
    /* Check for empty needle behavior.  */
    return !result || !memmem ("a", 1, 0, 0);]])],
        [gl_cv_func_memmem_works=yes], [gl_cv_func_memmem_works=no],
        [dnl Only glibc >= 2.9 and cygwin >= 1.7.0 are known to have a
         dnl memmem that works in linear time.
         AC_EGREP_CPP([Lucky user],
           [
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 9) || (__GLIBC__ > 2)
  Lucky user
 #endif
#endif
#ifdef __CYGWIN__
 #include <cygwin/version.h>
 #if CYGWIN_VERSION_DLL_MAJOR >= 1007
  Lucky user
 #endif
#endif
           ],
           [gl_cv_func_memmem_works=yes],
           [gl_cv_func_memmem_works="guessing no"])
        ])
      ])
    if test "$gl_cv_func_memmem_works" != yes; then
      REPLACE_MEMMEM=1
      AC_LIBOBJ([memmem])
    fi
  fi
]) # gl_FUNC_MEMMEM

# Prerequisites of lib/memmem.c.
AC_DEFUN([gl_PREREQ_MEMMEM], [:])
