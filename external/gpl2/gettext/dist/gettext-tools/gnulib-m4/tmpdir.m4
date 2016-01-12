# tmpdir.m4 serial 2 (gettext-0.15)
dnl Copyright (C) 2001-2002, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Prerequisites for lib/tmpdir.c

AC_DEFUN([gt_TMPDIR],
[
  AC_CHECK_FUNCS(__secure_getenv)
])
