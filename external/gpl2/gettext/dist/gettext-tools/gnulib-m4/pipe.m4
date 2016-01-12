# pipe.m4 serial 1
dnl Copyright (C) 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_PIPE],
[
  dnl Prerequisites of lib/pipe.h.
  AC_CHECK_HEADERS_ONCE(unistd.h)
  dnl Prerequisites of lib/pipe.c.
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_TYPE_MODE_T])
  AC_REQUIRE([AC_FUNC_FORK])
  AC_CHECK_FUNCS(posix_spawn)
])
