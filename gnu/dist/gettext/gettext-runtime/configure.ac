dnl Configuration for the gettext-runtime directory of GNU gettext
dnl Copyright (C) 1995-1999, 2000-2005 Free Software Foundation, Inc.
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
AC_CONFIG_SRCDIR(intl/dcigettext.c)
AC_CONFIG_AUX_DIR(../build-aux)
. $srcdir/../version.sh
AM_INIT_AUTOMAKE(gettext-runtime, $VERSION_NUMBER)
AM_CONFIG_HEADER(config.h)

dnl Installation directories.
AC_SUBST([docdir], ['${datadir}/doc/gettext'])
dnl Prefer modern Filesystem Hierarchy Standard over antiquated GNU standards.
test "$mandir" != '${prefix}/man' || mandir='${datadir}/man'

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_YACC

gt_JAVACOMP
AC_CHECK_PROG(JAR, jar, jar)
if test -n "$HAVE_JAVACOMP" && test -n "$JAR"; then
  BUILDJAVA=yes
else
  BUILDJAVA=no
fi
AC_SUBST(BUILDJAVA)

gt_CSHARPCOMP
if test -n "$HAVE_CSHARPCOMP" && test "$CSHARP_CHOICE" != no; then
  BUILDCSHARP=yes
else
  BUILDCSHARP=no
fi
AC_SUBST(BUILDCSHARP)

dnl Check for host type.
AC_CANONICAL_HOST

dnl Checks for UNIX variants that set DEFS,
AC_ISC_POSIX

dnl Checks for compiler output filename suffixes.
AC_OBJEXT
AC_EXEEXT

dnl Make sure we see all GNU and Solaris extensions.
AC_GNU_SOURCE
gl_USE_SYSTEM_EXTENSIONS

dnl Check for build configuration.
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_RELOCATABLE

dnl Checks for libraries.

dnl Checks for header files.
AC_CHECK_HEADERS(string.h)
AM_STDBOOL_H

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

dnl Checks for library functions.
gl_ALLOCSA
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([setlocale strerror])
AC_REPLACE_FUNCS([atexit memmove])
gl_GETOPT
gt_FUNC_SETENV
gl_FUNC_STRERROR
gl_FUNC_STRTOUL
gl_FUNC_GLIBC_UNLOCKED_IO
gl_ERROR
gl_PATHMAX
gl_FUNC_READLINK
gl_XREADLINK
gl_CANONICALIZE

dnl These are the only lines required to internationalize the package.
dnl (OK, not quite, the AC_CONFIG_FILES has also some parts.)
AM_GNU_GETTEXT(use-libtool, need-ngettext)

dnl Put some default definitions into config.h.
AH_BOTTOM([
/* On Windows, variables that may be in a DLL must be marked specially.  */
#if defined _MSC_VER && defined _DLL
# define DLL_VARIABLE __declspec (dllimport)
#else
# define DLL_VARIABLE
#endif

/* Extra OS/2 (emx+gcc) defines.  */
#ifdef __EMX__
# include "intl/os2compat.h"
#endif
])

dnl Check for tools needed for formatting the documentation.
ac_aux_dir_abs=`cd $ac_aux_dir && pwd`
AC_PATH_PROG(PERL, perl, $ac_aux_dir_abs/missing perl)

dnl Check for tools needed for formatting man pages.
CROSS_COMPILING=$cross_compiling
AC_SUBST(CROSS_COMPILING)

dnl Checks for optional programs for the tests/lang-* tests.
gt_PROG_ANSI_CXX

dnl Determine whether to build libasprintf.
AC_ARG_ENABLE(libasprintf,
  [  --disable-libasprintf   do not build libasprintf],
  [if test "$enableval" != no; then
     SUBDIR_libasprintf=libasprintf
   else
     SUBDIR_libasprintf=
   fi
  ], SUBDIR_libasprintf=libasprintf)
if test "$CXX" = ":"; then
  SUBDIR_libasprintf=
fi
AC_SUBST(SUBDIR_libasprintf)

AC_CONFIG_SUBDIRS(libasprintf)

AC_CONFIG_FILES([Makefile])

AC_CONFIG_FILES([doc/Makefile])

AC_CONFIG_FILES([intl/Makefile])

AC_CONFIG_FILES([intl-java/Makefile])

AC_CONFIG_FILES([intl-csharp/Makefile])

AC_CONFIG_FILES([lib/Makefile],
                [FIX_MAKEFILE_COMPILE])
AC_CONFIG_FILES([lib/javacomp.sh:../gettext-tools/lib/javacomp.sh.in])
AC_CONFIG_FILES([lib/csharpcomp.sh:../gettext-tools/lib/csharpcomp.sh.in])

AC_CONFIG_FILES([src/Makefile],
                [FIX_MAKEFILE_COMPILE])
AC_CONFIG_FILES([src/gettext.sh], [chmod a+x src/gettext.sh])

AC_CONFIG_FILES([po/Makefile.in])

AC_CONFIG_FILES([man/Makefile],
                [FIX_MAKEFILE_DISTRIB])
AC_CONFIG_FILES([man/x-to-1])

AC_CONFIG_FILES([m4/Makefile])

AC_OUTPUT
