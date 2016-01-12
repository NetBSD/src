# execute.m4 serial 1
dnl Copyright (C) 2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_EXECUTE],
[
  dnl Prerequisites of lib/execute.c.
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_TYPE_MODE_T])
  AC_CHECK_HEADERS_ONCE(unistd.h)
  AC_REQUIRE([AC_FUNC_FORK])
  AC_CHECK_FUNCS(posix_spawn)
])
