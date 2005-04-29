dnl Configuration for the GNU libasprintf library
dnl Copyright (C) 2002-2005 Free Software Foundation, Inc.
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

AC_PREREQ(2.52)
AC_INIT
AC_CONFIG_SRCDIR(vasprintf.c)
AC_CONFIG_AUX_DIR(../../build-aux)
AM_INIT_AUTOMAKE(libasprintf, 1.0)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

dnl Check for host type.
AC_CANONICAL_HOST

dnl Checks for compiler output filename suffixes.
AC_OBJEXT
AC_EXEEXT

dnl Check for build configuration.
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Checks for libraries.

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
bh_C_SIGNED
AC_TYPE_SIZE_T
gl_AC_TYPE_LONG_LONG
gt_TYPE_LONGDOUBLE
gt_TYPE_WCHAR_T
gt_TYPE_WINT_T
AC_CHECK_TYPE([ptrdiff_t], ,
  [AC_DEFINE([ptrdiff_t], [long],
     [Define as the type of the result of subtracting two pointers, if the system doesn't define it.])
  ])
gt_TYPE_INTMAX_T
gl_XSIZE

dnl Checks for library functions.

AC_FUNC_ALLOCA
dnl Define an additional variable used in the Makefile substitution.
AC_EGREP_CPP([Need own alloca], [
#if defined __GNUC__ || defined _MSC_VER || !HAVE_ALLOCA_H
  Need own alloca
#endif
  ],
  ALLOCA_H=alloca.h,
  ALLOCA_H=)
AC_SUBST([ALLOCA_H])

AC_CHECK_FUNCS([snprintf vasprintf wcslen])
AC_CHECK_DECLS([_snprintf])
gt_PRINTF_POSIX

dnl Check for tools needed for formatting the documentation.
ac_aux_dir_abs=`cd $ac_aux_dir && pwd`
AC_PATH_PROG(DVIPS, dvips, $ac_aux_dir_abs/missing dvips)
AC_PATH_PROG(TEXI2PDF, texi2pdf, $ac_aux_dir_abs/missing texi2pdf)
AC_PATH_PROG(PERL, perl, $ac_aux_dir_abs/missing perl)

AC_CONFIG_FILES([Makefile],
                [FIX_MAKEFILE_COMPILE
                 FIX_MAKEFILE_INFO])

AC_OUTPUT
