# mkdtemp.m4 serial 3
dnl Copyright (C) 2001-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gt_FUNC_MKDTEMP],
[
  AC_REPLACE_FUNCS(mkdtemp)
  if test $ac_cv_func_mkdtemp = no; then
    gl_PREREQ_MKDTEMP
  fi
])

# Prerequisites of lib/mkdtemp.c
AC_DEFUN([gl_PREREQ_MKDTEMP],
[
  AC_REQUIRE([AC_HEADER_STAT])
  AC_CHECK_HEADERS_ONCE(sys/time.h unistd.h)
  AC_CHECK_HEADERS(time.h)
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
  AC_CHECK_FUNCS(gettimeofday)
])
