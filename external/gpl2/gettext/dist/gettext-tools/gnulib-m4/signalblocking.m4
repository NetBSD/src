# signalblocking.m4 serial 4
dnl Copyright (C) 2001-2002, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Determine available signal blocking primitives. Three different APIs exist:
# 1) POSIX: sigemptyset, sigaddset, sigprocmask
# 2) SYSV: sighold, sigrelse
# 3) BSD: sigblock, sigsetmask
# For simplicity, here we check only for the POSIX signal blocking.
AC_DEFUN([gl_SIGNALBLOCKING],
[
  signals_not_posix=
  AC_EGREP_HEADER(sigset_t, signal.h, , signals_not_posix=1)
  if test -z "$signals_not_posix"; then
    AC_CHECK_FUNC(sigprocmask, [gl_cv_func_sigprocmask=1])
  fi
  if test -n "$gl_cv_func_sigprocmask"; then
    AC_DEFINE([HAVE_POSIX_SIGNALBLOCKING], 1,
      [Define to 1 if you have the sigset_t type and the sigprocmask function.])
  else
    AC_LIBOBJ([sigprocmask])
    gl_PREREQ_SIGPROCMASK
  fi
])

# Prerequisites of lib/sigprocmask.h and lib/sigprocmask.c.
AC_DEFUN([gl_PREREQ_SIGPROCMASK], [
  AC_CHECK_TYPES([sigset_t],
    [gl_cv_type_sigset_t=yes], [gl_cv_type_sigset_t=no],
    [#include <signal.h>
/* Mingw defines sigset_t not in <signal.h>, but in <sys/types.h>.  */
#include <sys/types.h>])
  if test $gl_cv_type_sigset_t = yes; then
    AC_DEFINE([HAVE_SIGSET_T], [1],
      [Define to 1 if you lack the sigprocmask function but have the sigset_t type.])
  fi
  AC_CHECK_FUNCS_ONCE(raise)
])
