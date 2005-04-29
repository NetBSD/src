dnl Configuration for the autoconf-lib-link directory of GNU gettext
dnl Copyright (C) 1995-1999, 2000-2003, 2005 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT
AC_CONFIG_SRCDIR(tests/rpath-1a)
AC_CONFIG_AUX_DIR(../build-aux)
. $srcdir/../version.sh
AM_INIT_AUTOMAKE(autoconf-lib-link, $VERSION_NUMBER)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl Check for host type.
AC_CANONICAL_HOST

dnl Checks for UNIX variants that set DEFS,

dnl Checks for compiler output filename suffixes.
AC_OBJEXT
AC_EXEEXT

dnl Check for build configuration.
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Checks for libraries.

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

dnl Checks for tests/rpathcfg.
AC_SUBST(GCC)
AC_SUBST(LD)
AC_SUBST(with_gnu_ld)

aclocaldir='${datadir}/aclocal'
AC_SUBST(aclocaldir)

AC_CONFIG_FILES([Makefile],
                [FIX_MAKEFILE_DISTRIB])

AC_CONFIG_FILES([m4/Makefile])

AC_CONFIG_FILES([tests/Makefile])

AC_OUTPUT
