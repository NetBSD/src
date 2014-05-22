dnl Process this file with autoconf to produce a configure script.

dnl Copyright (C) 2005-2013 Free Software Foundation, Inc.
dnl Contributed by Red Hat, Inc.
dnl 
dnl This file is part of the GNU simulators.
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
dnl
AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

AC_CHECK_HEADERS(getopt.h)

AC_ARG_ENABLE(cycle-accurate,
[  --disable-cycle-accurate ],
[case "${enableval}" in
yes | no) ;;
*)	AC_MSG_ERROR(bad value ${enableval} given for --enable-cycle-accurate option) ;;
esac])

AC_ARG_ENABLE(cycle-stats,
[  --disable-cycle-stats ],
[case "${enableval}" in
yes | no) ;;
*)	AC_MSG_ERROR(bad value ${enableval} given for --enable-cycle-stats option) ;;
esac])

echo enable_cycle_accurate is $enable_cycle_accurate
echo enable_cycle_stats is $enable_cycle_stats

if test "x${enable_cycle_accurate}" != xno; then
  AC_DEFINE([CYCLE_ACCURATE], 1, [--enable-cycle-accurate])

  if test "x${enable_cycle_stats}" != xno; then
    AC_DEFINE([CYCLE_STATS], 1, [--enable-cycle-stats])
  fi
else
  if test "x${enable_cycle_stats}" != xno; then
    AC_ERROR([cycle-stats not available without cycle-accurate])
  fi
fi

SIM_AC_OUTPUT
