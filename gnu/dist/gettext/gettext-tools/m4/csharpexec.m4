# csharpexec.m4 serial 1 (gettext-0.14)
dnl Copyright (C) 2003-2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Prerequisites of csharpexec.sh.
# Sets HAVE_CSHARPEXEC to nonempty if csharpexec.sh will work.

AC_DEFUN([gt_CSHARPEXEC],
[
  AC_REQUIRE([gt_CSHARP_CHOICE])
  AC_MSG_CHECKING([for C[#] program execution engine])
  AC_EGREP_CPP(yes, [
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  yes
#endif
], MONO_PATH_SEPARATOR=';', MONO_PATH_SEPARATOR=':')
  HAVE_CSHARPEXEC=1
  pushdef([AC_MSG_CHECKING],[:])dnl
  pushdef([AC_CHECKING],[:])dnl
  pushdef([AC_MSG_RESULT],[:])dnl
  AC_CHECK_PROG(HAVE_ILRUN_IN_PATH, ilrun, yes)
  AC_CHECK_PROG(HAVE_MONO_IN_PATH, mono, yes)
  popdef([AC_MSG_RESULT])dnl
  popdef([AC_CHECKING])dnl
  popdef([AC_MSG_CHECKING])dnl
  for impl in "$CSHARP_CHOICE" pnet mono no; do
    case "$impl" in
      pnet)
        if test -n "$HAVE_ILRUN_IN_PATH" \
           && ilrun --version >/dev/null 2>/dev/null; then
          HAVE_ILRUN=1
          ac_result="ilrun"
          break
        fi
        ;;
      mono)
        if test -n "$HAVE_MONO_IN_PATH" \
           && mono --version >/dev/null 2>/dev/null; then
          HAVE_MONO=1
          ac_result="mono"
          break
        fi
        ;;
      no)
        HAVE_CSHARPEXEC=
        ac_result="no"
        break
        ;;
    esac
  done
  AC_MSG_RESULT([$ac_result])
  AC_SUBST(MONO_PATH)
  AC_SUBST(MONO_PATH_SEPARATOR)
  AC_SUBST(HAVE_ILRUN)
  AC_SUBST(HAVE_MONO)
])
