dnl Process this file with autoconf to produce a configure script.

dnl Copyright (C) 2015-2016 Free Software Foundation, Inc.
dnl
dnl Contributed by Red Hat.
dnl
dnl This file is part of GDB.

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
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

AC_PREREQ(2.64)dnl
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

SIM_AC_OPTION_ENDIAN
## We use NONSTRICT_ALIGNMENT as the default because AArch64 only
## enforces 4-byte alignment, even for 8-byte reads/writes.  The
## common core does not support this, so we opt for non-strict
## alignment instead.
SIM_AC_OPTION_ALIGNMENT(NONSTRICT_ALIGNMENT,NONSTRICT_ALIGNMENT)
SIM_AC_OPTION_WARNINGS

SIM_AC_OUTPUT
