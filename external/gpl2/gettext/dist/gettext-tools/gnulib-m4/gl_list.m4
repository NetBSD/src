# gl_list.m4 serial 1
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_LIST],
[
  AC_REQUIRE([AC_C_INLINE])
  if test $ac_cv_c_inline != no; then
    AC_DEFINE([HAVE_INLINE], 1,
      [Define to 1 if the compiler supports one of the keywords 'inline', '__inline__', '__inline'.])
  fi
])
