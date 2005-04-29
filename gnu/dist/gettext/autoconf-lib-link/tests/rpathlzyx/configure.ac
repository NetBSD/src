dnl Configuration of a program that links with librpathz.
dnl
dnl Copyright (C) 2002-2003, 2005 Free Software Foundation, Inc.
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

AC_INIT(configure.ac)
AC_CONFIG_AUX_DIR(../../../build-aux)
AM_INIT_AUTOMAKE(gettext_rpathlzyx, 0)
AC_PROG_CC
AC_LIB_LINKFLAGS([rpathx])
AC_LIB_LINKFLAGS([rpathy],[rpathx])
AC_LIB_LINKFLAGS([rpathz],[rpathx rpathy])
LIBRPATHZ="$LIBRPATHZ $LIBRPATHY $LIBRPATHX"
AC_SUBST(global_top_auxdir)
AC_OUTPUT([Makefile])
