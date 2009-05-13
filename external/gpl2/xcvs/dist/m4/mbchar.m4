# mbchar.m4 serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl autoconf tests required for use of mbchar.m4
dnl From Bruno Haible.

AC_DEFUN([gl_MBCHAR],
[
  AC_LIBSOURCES([mbchar.c, mbchar.h])
  AC_CHECK_HEADERS_ONCE(wchar.h wctype.h)

  if test x$ac_cv_header_wchar_h = xyes && test x$ac_cv_header_wctype_h = xyes; then
  	AC_LIBOBJ(mbchar)
  fi

  AC_REQUIRE([AC_GNU_SOURCE])
  :
])
