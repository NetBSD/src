dnl
dnl   Copyright (C) 2012-2022 Free Software Foundation, Inc.
dnl
dnl This file is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; see the file COPYING3.  If not see
dnl <http://www.gnu.org/licenses/>.
dnl

dnl Make sure your module depends on `all-bfd' when invoking this macro.
AC_DEFUN([BFD_64_BIT], [dnl
# See whether 64-bit bfd lib has been enabled.
OLD_CPPFLAGS=$CPPFLAGS
# Put the old CPPFLAGS last, in case the user's CPPFLAGS point somewhere
# with bfd, with -I/foo/include.  We always want our bfd.
CPPFLAGS="-I${srcdir}/../include -I../bfd -I${srcdir}/../bfd $CPPFLAGS"
# Note we cannot cache the result of this check because BFD64 may change
# when a secondary target has been added or removed and we have no access
# to this information here.
AC_MSG_CHECKING([whether BFD is 64-bit])
AC_EGREP_CPP([HAVE_BFD64],
  AC_LANG_PROGRAM(
    [#include "bfd.h"],
    [dnl
#ifdef BFD64
HAVE_BFD64
#endif]),
  [have_64_bit_bfd=yes],
  [have_64_bit_bfd=no])
AC_MSG_RESULT([$have_64_bit_bfd])
CPPFLAGS=$OLD_CPPFLAGS])
