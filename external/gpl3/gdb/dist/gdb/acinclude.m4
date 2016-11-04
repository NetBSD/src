dnl written by Rob Savoye <rob@cygnus.com> for Cygnus Support
dnl major rewriting for Tcl 7.5 by Don Libes <libes@nist.gov>

# Keep these includes in sync with the aclocal_m4_deps list in
# Makefile.in.

sinclude(acx_configure_dir.m4)

# This gets GDB_AC_LIBMCHECK.
sinclude(libmcheck.m4)

# This gets GDB_AC_TRANSFORM.
sinclude(transform.m4)

# This gets AM_GDB_WARNINGS.
sinclude(warning.m4)

dnl gdb/configure.in uses BFD_NEED_DECLARATION, so get its definition.
sinclude(../bfd/bfd.m4)

dnl This gets the standard macros.
sinclude(../config/acinclude.m4)

dnl This gets AC_PLUGINS, needed by ACX_LARGEFILE.
sinclude(../config/plugins.m4)

dnl For ACX_LARGEFILE.
sinclude(../config/largefile.m4)

dnl For AM_SET_LEADING_DOT.
sinclude(../config/lead-dot.m4)

dnl This gets autoconf bugfixes.
sinclude(../config/override.m4)

dnl For ZW_GNU_GETTEXT_SISTER_DIR.
sinclude(../config/gettext-sister.m4)

dnl For AC_LIB_HAVE_LINKFLAGS.
sinclude(../config/lib-ld.m4)
sinclude(../config/lib-prefix.m4)
sinclude(../config/lib-link.m4)

dnl For ACX_PKGVERSION and ACX_BUGURL.
sinclude(../config/acx.m4)

dnl for TCL definitions
sinclude(../config/tcl.m4)

dnl For dependency tracking macros.
sinclude([../config/depstand.m4])

dnl For AM_LC_MESSAGES
sinclude([../config/lcmessage.m4])

dnl For AM_LANGINFO_CODESET.
sinclude([../config/codeset.m4])

sinclude([../config/iconv.m4])

sinclude([../config/zlib.m4])

m4_include([common/common.m4])

dnl For libiberty_INIT.
m4_include(libiberty.m4)

dnl For --enable-build-with-cxx and COMPILER.
m4_include(build-with-cxx.m4)

dnl For GDB_AC_PTRACE.
m4_include(ptrace.m4)

## ----------------------------------------- ##
## ANSIfy the C compiler whenever possible.  ##
## From Franc,ois Pinard                     ##
## ----------------------------------------- ##

# Copyright (C) 1996-2016 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.

# serial 1

# @defmac AC_PROG_CC_STDC
# @maindex PROG_CC_STDC
# @ovindex CC
# If the C compiler in not in ANSI C mode by default, try to add an option
# to output variable @code{CC} to make it so.  This macro tries various
# options that select ANSI C on some system or another.  It considers the
# compiler to be in ANSI C mode if it handles function prototypes correctly.
#
# If you use this macro, you should check after calling it whether the C
# compiler has been set to accept ANSI C; if not, the shell variable
# @code{am_cv_prog_cc_stdc} is set to @samp{no}.  If you wrote your source
# code in ANSI C, you can make an un-ANSIfied copy of it by using the
# program @code{ansi2knr}, which comes with Ghostscript.
# @end defmac

AC_DEFUN([AM_PROG_CC_STDC],
[AC_REQUIRE([AC_PROG_CC])
AC_BEFORE([$0], [AC_C_INLINE])
AC_BEFORE([$0], [AC_C_CONST])
dnl Force this before AC_PROG_CPP.  Some cpp's, eg on HPUX, require
dnl a magic option to avoid problems with ANSI preprocessor commands
dnl like #elif.
dnl FIXME: can't do this because then AC_AIX won't work due to a
dnl circular dependency.
dnl AC_BEFORE([$0], [AC_PROG_CPP])
AC_MSG_CHECKING([for ${CC-cc} option to accept ANSI C])
AC_CACHE_VAL(am_cv_prog_cc_stdc,
[am_cv_prog_cc_stdc=no
ac_save_CC="$CC"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX 10.20 and later	-Ae
# HP-UX older versions	-Aa -D_HPUX_SOURCE
# SVR4			-Xc -D__EXTENSIONS__
for ac_arg in "" -qlanglvl=ansi -std1 -Ae "-Aa -D_HPUX_SOURCE" "-Xc -D__EXTENSIONS__"
do
  CC="$ac_save_CC $ac_arg"
  AC_TRY_COMPILE(
[#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Most of the following tests are stolen from RCS 5.7's src/conf.sh.  */
struct buf { int x; };
FILE * (*rcsopen) (struct buf *, struct stat *, int);
static char *e (p, i)
     char **p;
     int i;
{
  return p[i];
}
static char *f (char * (*g) (char **, int), char **p, ...)
{
  char *s;
  va_list v;
  va_start (v,p);
  s = g (p, va_arg (v,int));
  va_end (v);
  return s;
}
int test (int i, double x);
struct s1 {int (*f) (int a);};
struct s2 {int (*f) (double a);};
int pairnames (int, char **, FILE *(*)(struct buf *, struct stat *, int), int, int);
int argc;
char **argv;
], [
return f (e, argv, 0) != argv[0]  ||  f (e, argv, 1) != argv[1];
],
[am_cv_prog_cc_stdc="$ac_arg"; break])
done
CC="$ac_save_CC"
])
if test -z "$am_cv_prog_cc_stdc"; then
  AC_MSG_RESULT([none needed])
else
  AC_MSG_RESULT([$am_cv_prog_cc_stdc])
fi
case "x$am_cv_prog_cc_stdc" in
  x|xno) ;;
  *) CC="$CC $am_cv_prog_cc_stdc" ;;
esac
])

dnl written by Guido Draheim <guidod@gmx.de>, original by Alexandre Oliva 
dnl Version 1.3 (2001/03/02)
dnl source http://www.gnu.org/software/ac-archive/Miscellaneous/ac_define_dir.html

AC_DEFUN([AC_DEFINE_DIR], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_define_dir=`eval echo [$]$2`
  ac_define_dir=`eval echo [$]ac_define_dir`
  ifelse($3, ,
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir"),
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir", $3))
])

dnl See whether we need a declaration for a function.
dnl The result is highly dependent on the INCLUDES passed in, so make sure
dnl to use a different cache variable name in this macro if it is invoked
dnl in a different context somewhere else.
dnl gcc_AC_CHECK_DECL(SYMBOL,
dnl 	[ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, INCLUDES]]])
AC_DEFUN([gcc_AC_CHECK_DECL],
[AC_MSG_CHECKING([whether $1 is declared])
AC_CACHE_VAL(gcc_cv_have_decl_$1,
[AC_TRY_COMPILE([$4],
[#ifndef $1
char *(*pfn) = (char *(*)) $1 ;
#endif], eval "gcc_cv_have_decl_$1=yes", eval "gcc_cv_have_decl_$1=no")])
if eval "test \"`echo '$gcc_cv_have_decl_'$1`\" = yes"; then
  AC_MSG_RESULT(yes) ; ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no) ; ifelse([$3], , :, [$3])
fi
])dnl

dnl Check multiple functions to see whether each needs a declaration.
dnl Arrange to define HAVE_DECL_<FUNCTION> to 0 or 1 as appropriate.
dnl gcc_AC_CHECK_DECLS(SYMBOLS,
dnl 	[ACTION-IF-NEEDED [, ACTION-IF-NOT-NEEDED [, INCLUDES]]])
AC_DEFUN([gcc_AC_CHECK_DECLS],
[for ac_func in $1
do
changequote(, )dnl
  ac_tr_decl=HAVE_DECL_`echo $ac_func | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
changequote([, ])dnl
gcc_AC_CHECK_DECL($ac_func,
  [AC_DEFINE_UNQUOTED($ac_tr_decl, 1) $2],
  [AC_DEFINE_UNQUOTED($ac_tr_decl, 0) $3],
dnl It is possible that the include files passed in here are local headers
dnl which supply a backup declaration for the relevant prototype based on
dnl the definition of (or lack of) the HAVE_DECL_ macro.  If so, this test
dnl will always return success.  E.g. see libiberty.h's handling of
dnl `basename'.  To avoid this, we define the relevant HAVE_DECL_ macro to
dnl 1 so that any local headers used do not provide their own prototype
dnl during this test.
#undef $ac_tr_decl
#define $ac_tr_decl 1
  $4
)
done
dnl Automatically generate config.h entries via autoheader.
if test x = y ; then
  patsubst(translit([$1], [a-z], [A-Z]), [\w+],
    [AC_DEFINE([HAVE_DECL_\&], 1,
      [Define to 1 if we found this declaration otherwise define to 0.])])dnl
fi
])

dnl Find the location of the private Tcl headers
dnl When Tcl is installed, this is TCL_INCLUDE_SPEC/tcl-private/generic
dnl When Tcl is in the build tree, this is not needed.
dnl
dnl Note: you must use first use SC_LOAD_TCLCONFIG!
AC_DEFUN([CY_AC_TCL_PRIVATE_HEADERS], [
  AC_MSG_CHECKING([for Tcl private headers])
  private_dir=""
  dir=`echo ${TCL_INCLUDE_SPEC}/tcl-private/generic | sed -e s/-I//`
  if test -f ${dir}/tclInt.h ; then
    private_dir=${dir}
  fi

  if test x"${private_dir}" = x; then
    AC_ERROR(could not find private Tcl headers)
  else
    TCL_PRIVATE_INCLUDE="-I${private_dir}"
    AC_MSG_RESULT(${private_dir})
  fi
])

dnl Find the location of the private Tk headers
dnl When Tk is installed, this is TK_INCLUDE_SPEC/tk-private/generic
dnl When Tk is in the build tree, this not needed.
dnl
dnl Note: you must first use SC_LOAD_TKCONFIG
AC_DEFUN([CY_AC_TK_PRIVATE_HEADERS], [
  AC_MSG_CHECKING([for Tk private headers])
  private_dir=""
  dir=`echo ${TK_INCLUDE_SPEC}/tk-private/generic | sed -e s/-I//`
  if test -f ${dir}/tkInt.h; then
    private_dir=${dir}
  fi

  if test x"${private_dir}" = x; then
    AC_ERROR(could not find Tk private headers)
  else
    TK_PRIVATE_INCLUDE="-I${private_dir}"
    AC_MSG_RESULT(${private_dir})
  fi
])

dnl GDB_AC_DEFINE_RELOCATABLE([VARIABLE], [ARG-NAME], [SHELL-VARIABLE])
dnl For use in processing directory values for --with-foo.
dnl If the path in SHELL_VARIABLE is relative to the prefix, then the
dnl result is relocatable, then this will define the C macro
dnl VARIABLE_RELOCATABLE to 1; otherwise it is defined as 0.
AC_DEFUN([GDB_AC_DEFINE_RELOCATABLE], [
  if test "x$exec_prefix" = xNONE || test "x$exec_prefix" = 'x${prefix}'; then
     if test "x$prefix" = xNONE; then
     	test_prefix=/usr/local
     else
	test_prefix=$prefix
     fi
  else
     test_prefix=$exec_prefix
  fi
  value=0
  case [$3] in
     "${test_prefix}"|"${test_prefix}/"*|\
	'${exec_prefix}'|'${exec_prefix}/'*)
     value=1
     ;;
  esac
  AC_DEFINE_UNQUOTED([$1]_RELOCATABLE, $value, [Define if the $2 directory should be relocated when GDB is moved.])
])

dnl GDB_AC_WITH_DIR([VARIABLE], [ARG-NAME], [HELP], [DEFAULT])
dnl Add a new --with option that defines a directory.
dnl The result is stored in VARIABLE.  AC_DEFINE_DIR is called
dnl on this variable, as is AC_SUBST.
dnl ARG-NAME is the base name of the argument (without "--with").
dnl HELP is the help text to use.
dnl If the user's choice is relative to the prefix, then the
dnl result is relocatable, then this will define the C macro
dnl VARIABLE_RELOCATABLE to 1; otherwise it is defined as 0.
dnl DEFAULT is the default value, which is used if the user
dnl does not specify the argument.
AC_DEFUN([GDB_AC_WITH_DIR], [
  AC_ARG_WITH([$2], AS_HELP_STRING([--with-][$2][=PATH], [$3]), [
    [$1]=$withval], [[$1]=[$4]])
  AC_DEFINE_DIR([$1], [$1], [$3])
  AC_SUBST([$1])
  GDB_AC_DEFINE_RELOCATABLE([$1], [$2], ${ac_define_dir})
  ])

dnl GDB_AC_CHECK_BFD([MESSAGE], [CV], [CODE], [HEADER])
dnl Check whether BFD provides a feature.
dnl MESSAGE is the "checking" message to display.
dnl CV is the name of the cache variable where the result is stored.
dnl The result will be "yes" or "no".
dnl CODE is some code to compile that checks for the feature.
dnl A link test is run.
dnl HEADER is the name of an extra BFD header to include.
AC_DEFUN([GDB_AC_CHECK_BFD], [
  OLD_CFLAGS=$CFLAGS
  OLD_LDFLAGS=$LDFLAGS
  OLD_LIBS=$LIBS
  # Put the old CFLAGS/LDFLAGS last, in case the user's (C|LD)FLAGS
  # points somewhere with bfd, with -I/foo/lib and -L/foo/lib.  We
  # always want our bfd.
  CFLAGS="-I${srcdir}/../include -I../bfd -I${srcdir}/../bfd $CFLAGS"
  ZLIBDIR=`echo $zlibdir | sed 's,\$(top_builddir)/,,g'`
  LDFLAGS="-L../bfd -L../libiberty $ZLIBDIR $LDFLAGS"
  intl=`echo $LIBINTL | sed 's,${top_builddir}/,,g'`
  LIBS="-lbfd -liberty -lz $intl $LIBS"
  AC_CACHE_CHECK([$1], [$2],
  [AC_TRY_LINK(
  [#include <stdlib.h>
  #include "bfd.h"
  #include "$4"
  ],
  [return $3;], [[$2]=yes], [[$2]=no])])
  CFLAGS=$OLD_CFLAGS
  LDFLAGS=$OLD_LDFLAGS
  LIBS=$OLD_LIBS])

dnl GDB_GUILE_PROGRAM_NAMES([PKG-CONFIG], [VERSION])
dnl
dnl Define and substitute 'GUILD' to contain the absolute file name of
dnl the 'guild' command for VERSION, using PKG-CONFIG.  (This is
dnl similar to Guile's 'GUILE_PROGS' macro.)
AC_DEFUN([GDB_GUILE_PROGRAM_NAMES], [
  AC_CACHE_CHECK([for the absolute file name of the 'guild' command],
    [ac_cv_guild_program_name],
    [ac_cv_guild_program_name="`$1 --variable guild $2`"

     # In Guile up to 2.0.11 included, guile-2.0.pc would not define
     # the 'guild' and 'bindir' variables.  In that case, try to guess
     # what the program name is, at the risk of getting it wrong if
     # Guile was configured with '--program-suffix' or similar.
     if test "x$ac_cv_guild_program_name" = "x"; then
       guile_exec_prefix="`$1 --variable exec_prefix $2`"
       ac_cv_guild_program_name="$guile_exec_prefix/bin/guild"
     fi
  ])

  if ! "$ac_cv_guild_program_name" --version >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD; then
    AC_MSG_ERROR(['$ac_cv_guild_program_name' appears to be unusable])
  fi

  GUILD="$ac_cv_guild_program_name"
  AC_SUBST([GUILD])
])

dnl GDB_GUILD_TARGET_FLAG
dnl
dnl Compute the value of GUILD_TARGET_FLAG.
dnl For native builds this is empty.
dnl For cross builds this is --target=<host>.
AC_DEFUN([GDB_GUILD_TARGET_FLAG], [
  if test "$cross_compiling" = no; then
    GUILD_TARGET_FLAG=
  else
    GUILD_TARGET_FLAG="--target=$host"
  fi
  AC_SUBST(GUILD_TARGET_FLAG)
])

dnl GDB_TRY_GUILD([SRC-FILE])
dnl
dnl We precompile the .scm files and install them with gdb, so make sure
dnl guild works for this host.
dnl The .scm files are precompiled for several reasons:
dnl 1) To silence Guile during gdb startup (Guile's auto-compilation output
dnl    is unnecessarily verbose).
dnl 2) Make gdb developers see compilation errors/warnings during the build,
dnl    and not leave it to later when the user runs gdb.
dnl 3) As a convenience for the user, so that one copy of the files is built
dnl    instead of one copy per user.
dnl
dnl Make sure guild can handle this host by trying to compile SRC-FILE, and
dnl setting ac_cv_guild_ok to yes or no.
dnl Note that guild can handle cross-compilation.
dnl It could happen that guild can't handle the host, but guile would still
dnl work.  For the time being we're conservative, and if guild doesn't work
dnl we punt.
AC_DEFUN([GDB_TRY_GUILD], [
  AC_REQUIRE([GDB_GUILD_TARGET_FLAG])
  AC_CACHE_CHECK([whether guild supports this host],
    [ac_cv_guild_ok],
    [echo "$ac_cv_guild_program_name compile $GUILD_TARGET_FLAG -o conftest.go $1" >&AS_MESSAGE_LOG_FD
     if "$ac_cv_guild_program_name" compile $GUILD_TARGET_FLAG -o conftest.go "$1" >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD; then
       ac_cv_guild_ok=yes
     else
       ac_cv_guild_ok=no
     fi])
])
