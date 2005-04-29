dnl Configuration for the gettext-tools directory of GNU gettext
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
AC_CONFIG_SRCDIR(src/msgfmt.c)
AC_CONFIG_AUX_DIR(../build-aux)
. $srcdir/../version.sh
AM_INIT_AUTOMAKE(gettext-tools, $VERSION_NUMBER)
AM_CONFIG_HEADER(config.h)

dnl Installation directories.
AC_SUBST([localedir], ['${datadir}/locale'])
AC_SUBST([docdir], ['${datadir}/doc/gettext'])
dnl Prefer modern Filesystem Hierarchy Standard over antiquated GNU standards.
test "$mandir" != '${prefix}/man' || mandir='${datadir}/man'

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_YACC

gt_GCJ
if test -n "$HAVE_GCJ"; then
  BUILDJAVAEXE=yes
else
  BUILDJAVAEXE=no
fi
AC_SUBST(BUILDJAVAEXE)

gt_JAVACOMP
AC_CHECK_PROG(JAR, jar, jar)
if test -n "$HAVE_JAVACOMP" && test -n "$JAR"; then
  BUILDJAVA=yes
else
  BUILDJAVA=no
fi
AC_SUBST(BUILDJAVA)

gt_JAVAEXEC
if test -n "$HAVE_JAVAEXEC" && test $BUILDJAVA = yes; then
  TESTJAVA=yes
else
  TESTJAVA=no
fi
AC_SUBST(TESTJAVA)

gt_CSHARPCOMP
if test -n "$HAVE_CSHARPCOMP" && test "$CSHARP_CHOICE" != no; then
  BUILDCSHARP=yes
else
  BUILDCSHARP=no
fi
AC_SUBST(BUILDCSHARP)

gt_CSHARPEXEC
if test -n "$HAVE_CSHARPEXEC" && test $BUILDCSHARP = yes; then
  TESTCSHARP=yes
else
  TESTCSHARP=no
fi
AC_SUBST(TESTCSHARP)

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
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h malloc.h pwd.h string.h unistd.h utime.h values.h)
AM_STDBOOL_H

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
bh_C_SIGNED
gl_AC_TYPE_UNSIGNED_LONG_LONG
AC_TYPE_SIZE_T
AC_CHECK_TYPES(ptrdiff_t)
gl_AC_TYPE_UINTMAX_T
gt_TYPE_SSIZE_T
AC_TYPE_PID_T
AC_TYPE_MODE_T
gt_TYPE_SIG_ATOMIC_T

dnl Checks for library functions.
gl_ALLOCSA
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([chown getcwd posix_spawn raise select strerror strtoul uname \
utime utimes waitid])
AC_REPLACE_FUNCS([atexit memmove memset stpcpy strcspn \
strpbrk strstr vasprintf])
AM_FUNC_GETLINE
if test $am_cv_func_working_getline != yes; then
  AC_CHECK_FUNCS(getdelim)
fi
gl_EXITFAIL
gl_FUNC_FNMATCH_POSIX
gl_GETOPT
gl_FUNC_EACCESS
gl_FUNC_STPNCPY
gl_STRCASE
gl_FUNC_STRTOUL
gl_MBSWIDTH
gt_PREREQ_BACKUPFILE
AC_FUNC_VFORK
gt_UNION_WAIT
gt_TMPDIR
gt_FUNC_MKDTEMP
gt_SIGNALBLOCKING
gt_SIGINFO
gt_FUNC_SETENV
gl_FUNC_STRERROR
gl_FUNC_GLIBC_UNLOCKED_IO
gl_ERROR
gl_QUOTEARG
gl_QUOTE
gl_PATHMAX
gl_FUNC_READLINK
gl_XREADLINK
gl_CANONICALIZE
gt_SETLOCALE
gl_XSIZE
gt_LIBGREP

gt_PREREQ_HOSTNAME

dnl These are the only lines required to internationalize the package.
dnl (OK, not quite, the AC_CONFIG_FILES has also some parts.)
AM_GNU_GETTEXT(use-libtool, need-ngettext)

dnl Put some default definitions into config.h.
AH_TOP([
/* Default value for alignment of strings in .mo file.  */
#define DEFAULT_OUTPUT_ALIGNMENT 1
])
AH_BOTTOM([
/* A file name cannot consist of any character possible.  INVALID_PATH_CHAR
   contains the characters not allowed.  */
#if defined _MSC_VER || defined __MINGW32__
/* Woe32.  This string is valid for Windows NT/2000.  On Windows 95/98/ME some
   few characters in the range 0x80..0xff are invalid as well, but this doesn't
   matter much for our purposes.  */
# define INVALID_PATH_CHAR "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\"*/:<>?\\|"
#elif defined MSDOS
/* Something like this for MSDOG.  */
# define INVALID_PATH_CHAR "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 \177\\:."
#else
/* Unix.  */
# define INVALID_PATH_CHAR "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 \177/"
#endif

/* This is the page width for the message_print function.  It should
   not be set to more than 79 characters (Emacs users will appreciate
   it).  It is used to wrap the msgid and msgstr strings, and also to
   wrap the file position (#:) comments.  */
#define PAGE_WIDTH 79

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

dnl Check for the expat XML parser.
dnl On operating systems where binary distribution vendors are likely to
dnl ship both gettext and expat, we use dynamic loading to avoid a hard
dnl dependency from gettext to expat.
case "$host_os" in
  linux*)
    AC_DEFINE([DYNLOAD_LIBEXPAT], 1,
      [Define to 1 if libexpat shall be dynamically loaded via dlopen().])
    LIBEXPAT="-ldl"
    LTLIBEXPAT="-ldl"
    AC_SUBST(LIBEXPAT)
    AC_SUBST(LTLIBEXPAT)
    ;;
  *)
    AC_LIB_HAVE_LINKFLAGS([expat], [],
      [#include <expat.h>], [XML_ExpatVersion();])
    ;;
esac

dnl Check for Emacs and where to install .elc files.
AM_PATH_LISPDIR

aclocaldir='${datadir}/aclocal'
AC_SUBST(aclocaldir)

dnl Check for tools needed for formatting the documentation.
ac_aux_dir_abs=`cd $ac_aux_dir && pwd`
AC_PATH_PROG(DVIPS, dvips, $ac_aux_dir_abs/missing dvips)
AC_PATH_PROG(TEXI2PDF, texi2pdf, $ac_aux_dir_abs/missing texi2pdf)
AC_PATH_PROG(PERL, perl, $ac_aux_dir_abs/missing perl)

dnl Check for tools needed for formatting man pages.
CROSS_COMPILING=$cross_compiling
AC_SUBST(CROSS_COMPILING)

dnl Checks for optional programs for the tests/lang-* tests.
gt_PROG_ANSI_CXX
gt_LOCALE_FR
gt_LOCALE_FR_UTF8

dnl Check whether to execute tests that rely on libasprintf.
dnl This test must be consistent with gettext-runtime/configure.ac.
if test "${enable_libasprintf+set}" = set; then
  if test "$enable_libasprintf" != no; then
    TESTLIBASPRINTF=yes
  else
    TESTLIBASPRINTF=no
  fi
else
  TESTLIBASPRINTF=yes
fi
if test "$CXX" = ":"; then
  TESTLIBASPRINTF=no
fi
AC_SUBST(TESTLIBASPRINTF)

dnl Generate the version information file in the intl/ directory.
test -d intl || mkdir intl
echo "GNU gettext library from gettext-$VERSION" > intl/VERSION
cat > intl/ChangeLog.inst <<EOF
$RELEASE_DATE  GNU  <bug-gnu-gettext@gnu.org>

	* Version $VERSION released.

EOF

AC_CONFIG_FILES([Makefile])

AC_CONFIG_FILES([doc/Makefile],
                [FIX_MAKEFILE_COMPILE
                 FIX_MAKEFILE_DISTRIB
                 FIX_MAKEFILE_INFO])

AC_CONFIG_FILES([intl/Makefile:../gettext-runtime/intl/Makefile.in], [
  # Change srcdir variable so that it points to ../gettext-runtime/intl.
  sed -e 's|^srcdir =.*$|srcdir = $(top_srcdir)/../gettext-runtime/intl|' \
    < intl/Makefile > intl/Makefile.tmp
  mv intl/Makefile.tmp intl/Makefile
  ])

AC_CONFIG_FILES([lib/Makefile],
                [FIX_MAKEFILE_COMPILE])
AC_CONFIG_FILES([lib/javacomp.sh lib/javaexec.sh])
AC_CONFIG_FILES([lib/csharpcomp.sh lib/csharpexec.sh])

AC_CONFIG_FILES([libgrep/Makefile],
                [FIX_MAKEFILE_COMPILE])

AC_CONFIG_FILES([libuniname/Makefile],
                [FIX_MAKEFILE_COMPILE])

AC_CONFIG_FILES([src/Makefile],
                [FIX_MAKEFILE_COMPILE])
AC_CONFIG_FILES([src/user-email:src/user-email.sh.in])

AC_CONFIG_FILES([po/Makefile.in])

AC_CONFIG_FILES([projects/Makefile])

AC_CONFIG_FILES([misc/Makefile])
AC_CONFIG_FILES([misc/gettextize], [chmod a+x misc/gettextize])
AC_CONFIG_FILES([misc/autopoint], [chmod a+x misc/autopoint])

AC_CONFIG_FILES([man/Makefile],
                [FIX_MAKEFILE_DISTRIB])
AC_CONFIG_FILES([man/x-to-1])

AC_CONFIG_FILES([m4/Makefile])

AC_CONFIG_FILES([tests/Makefile],
                [FIX_MAKEFILE_COMPILE])

AC_CONFIG_FILES([examples/Makefile],
                [FIX_MAKEFILE_DISTRIB])
AC_CONFIG_FILES([examples/installpaths])
AC_CONFIG_FILES([examples/po/Makefile], [AM_POSTPROCESS_PO_MAKEFILE])

AC_OUTPUT
