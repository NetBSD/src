dnl Autoconf configure snippets for common.
dnl Copyright (C) 1995-2014 Free Software Foundation, Inc.
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

dnl Invoke configury needed by the files in 'common'.
AC_DEFUN([GDB_AC_COMMON], [
  AC_HEADER_STDC
  AC_FUNC_ALLOCA

  dnl Note that this requires codeset.m4, which is included
  dnl by the users of common.m4.
  AM_LANGINFO_CODESET

  AC_CHECK_HEADERS(linux/perf_event.h locale.h memory.h signal.h dnl
		   sys/resource.h sys/socket.h sys/syscall.h dnl
		   sys/un.h sys/wait.h dnl
		   thread_db.h wait.h)

  AC_CHECK_FUNCS([fdwalk getrlimit pipe pipe2 socketpair])

  AC_CHECK_DECLS([strerror, strstr])
])
