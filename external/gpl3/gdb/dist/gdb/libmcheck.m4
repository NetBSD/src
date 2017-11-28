dnl Copyright (C) 2012-2017 Free Software Foundation, Inc.
dnl
dnl This file is part of GDB.
dnl
dnl This program is free software; you can redistribute it and/or modify
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
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl GDB_AC_LIBMCHECK([DEFAULT])
dnl Provide an --enable-libmcheck/--disable-libmcheck set of options
dnl allowing a user to enable this option even when building releases,
dnl or to disable it when building a snapshot.
dnl DEFAULT (yes/no) is used as default if the user doesn't set
dnl the option explicitly.

AC_DEFUN([GDB_AC_LIBMCHECK],
[
  AC_ARG_ENABLE(libmcheck,
    AS_HELP_STRING([--enable-libmcheck],
		   [Try linking with -lmcheck if available]),
    [case "${enableval}" in
      yes | y) ENABLE_LIBMCHECK="yes" ;;
      no | n)  ENABLE_LIBMCHECK="no" ;;
      *) AC_MSG_ERROR(bad value ${enableval} for --enable-libmcheck) ;;
    esac])

  if test -z "${ENABLE_LIBMCHECK}"; then
    ENABLE_LIBMCHECK=[$1]
  fi

  if test "$ENABLE_LIBMCHECK" = "yes" ; then
    AC_CHECK_LIB(mcheck, main)
  fi
])
