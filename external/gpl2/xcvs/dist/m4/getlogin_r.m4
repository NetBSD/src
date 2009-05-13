#serial 1

# Copyright (C) 2005 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Derek Price
dnl
dnl Provide getlogin_r when the system lacks it.
dnl

AC_DEFUN([gl_GETLOGIN_R_SUBSTITUTE],
[
  gl_PREREQ_GETLOGIN_R
  AC_LIBSOURCE([getlogin_r.h])
  AC_LIBOBJ([getlogin_r])
])

AC_DEFUN([gl_GETLOGIN_R],
[
  AC_REPLACE_FUNCS([getlogin_r])
  if test $ac_cv_func_getlogin_r = no; then
    gl_GETLOGIN_R_SUBSTITUTE
  fi
])

AC_DEFUN([gl_PREREQ_GETLOGIN_R],
[
  AC_CHECK_HEADERS_ONCE([unistd.h])
  AC_CHECK_DECLS_ONCE([getlogin getlogin_r])
])
