#serial 4

# Copyright (C) 2002 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_BISON],
[dnl
dnl All this cruft, which tries to fail harmlessly when bison is not present,
dnl will no longer be necessary after we require an Automake release 1.10 or
dnl later, as those avoid generating .c files from .y when not in maintainer
dnl mode.  This is currently a fork from the GNULIB version of this file.
  # expand $ac_aux_dir to an absolute path
  am_aux_dir=`cd $ac_aux_dir && pwd`

  # getdate.y works with bison only.
  : ${YACC="\${SHELL} $am_aux_dir/bison-missing --run bison -y"}
dnl
dnl Declaring YACC & YFLAGS precious will not be necessary after GNULIB
dnl requires an Autoconf greater than 2.59c, but it will probably still be
dnl useful to override the description of YACC in the --help output, re
dnl getdate.y assuming `bison -y'.
  AC_ARG_VAR(YACC,
[The `Yet Another C Compiler' implementation to use.  Defaults to `bison -y'.
Values other than `bison -y' will most likely break on most systems.])dnl
  AC_ARG_VAR(YFLAGS,
[YFLAGS contains the list arguments that will be passed by default to Bison.
This script will default YFLAGS to the empty string to avoid a default value of
`-d' given by some make applications.])dnl
])
