dnl
dnl aclocal.m4 --- autoconf input file for gawk
dnl 
dnl Copyright (C) 1995, 96 the Free Software Foundation, Inc.
dnl 
dnl This file is part of GAWK, the GNU implementation of the
dnl AWK Progamming Language.
dnl 
dnl GAWK is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl 
dnl GAWK is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
dnl

dnl gawk-specific macros for autoconf. one day hopefully part of autoconf

AC_DEFUN(GAWK_AC_C_STRINGIZE, [
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([for ANSI stringizing capability])
AC_CACHE_VAL(gawk_cv_c_stringize, 
AC_EGREP_CPP([#teststring],[
#define x(y) #y

char *s = x(teststring);
], gawk_cv_c_stringize=no, gawk_cv_c_stringize=yes))
if test "${gawk_cv_c_stringize}" = yes
then
	AC_DEFINE(HAVE_STRINGIZE)
fi
AC_MSG_RESULT([${gawk_cv_c_stringize}])
])dnl
