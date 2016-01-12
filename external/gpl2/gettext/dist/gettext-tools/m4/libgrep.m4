# libgrep.m4 serial 2 (gettext-0.16)
dnl Copyright (C) 2005-2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gt_LIBGREP],
[
  AC_REQUIRE([AM_STDBOOL_H])
  AC_REQUIRE([gl_FUNC_MBRTOWC])
  AC_CHECK_FUNC([wcscoll], ,
    [AC_DEFINE([wcscoll], [wcscmp],
       [Define to wcscmp if the function wcscoll does not exist.])])
  m4_pushdef([AC_LIBOBJ], m4_defn([gt_LIBGREP_OBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gt_LIBGREP_REPLACE_FUNCS]))
  gl_HARD_LOCALE
  gl_FUNC_MEMCHR
  gl_FUNC_STRDUP
  gl_INCLUDED_REGEX([libgrep/regex.c])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_SUBST([LIBGREPOBJS])
])

# Like AC_LIBOBJ, except that the module name goes into LIBGREPOBJS
# instead of into LIBOBJS.
AC_DEFUN([gt_LIBGREP_OBJ],
  [LIBGREPOBJS="$LIBGREPOBJS $1.$ac_objext"])

# Like AC_REPLACE_FUNCS, except that the module name goes into LIBGREPOBJS
# instead of into LIBOBJS.
AC_DEFUN([gt_LIBGREP_REPLACE_FUNCS],
  [AC_CHECK_FUNCS([$1], , [gt_LIBGREP_OBJ($ac_func)])])
