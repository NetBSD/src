dnl Process this file with autoconf to produce a configure script.
dnl Id: configure.ac,v 1.22 2003/02/05 00:40:19 karl Exp
dnl
dnl This file is free software; as a special exception the author gives
dnl unlimited permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
dnl
AC_PREREQ(2.57)dnl Minimum Autoconf version required.
dnl
AC_INIT([GNU Texinfo], [4.5])
AC_CONFIG_SRCDIR([makeinfo/makeinfo.c])
AM_CONFIG_HEADER(config.h:config.in)dnl Keep filename to 8.3 for MS-DOS.
AM_INIT_AUTOMAKE([dist-bzip2])
dnl
AM_MAINTAINER_MODE
AM_CONDITIONAL(TEXINFO_MAINT, test -n "$TEXINFO_MAINT")

dnl Checks for programs.
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_ISC_POSIX
AC_MINIX

# Needed on sysV68 for sigblock, sigsetmask.  But check for it in libc first.
AC_CHECK_FUNC(sigblock, , AC_CHECK_LIB(bsd, sigblock))

dnl Checks for header files.
dnl Do not use <ncurses/termcap.h> unless we're linking with ncurses.
if test "x$termlib" = xncurses; then
  dnl Use AC_CHECK_HEADERS so the HAVE_*_H symbol gets defined.
  AC_CHECK_HEADERS(ncurses/termcap.h)
fi
AC_HEADER_STAT
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h io.h limits.h pwd.h string.h strings.h \
  termcap.h termio.h termios.h unistd.h \
  sys/fcntl.h sys/file.h sys/stream.h sys/time.h sys/ttold.h sys/wait.h)

dnl sys/ptem.h requires sys/stream.h for mblk_t on Solaris.
AC_CHECK_HEADERS([sys/ptem.h],,,
[#if HAVE_SYS_STREAM_H
#include <sys/stream.h>
#endif
])

AC_SYS_POSIX_TERMIOS
AC_HEADER_TIOCGWINSZ

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIGNAL
AC_C_CONST
AC_STRUCT_TM

dnl Checks for function declarations.
txi_CHECK_DECLS

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_STRCOLL
AC_FUNC_VPRINTF
dnl in theory only pre-sysvr3 systems needed this and it's not likely
dnl that anyone compiling new texinfo still has such a thing? we'll see.
dnl AC_FUNC_SETVBUF_REVERSED
AC_CHECK_FUNCS(bzero getcwd memset setvbuf sigaction sigprocmask \
               sigsetmask strchr)
dnl strerror, xmalloc, xrealloc, probably others should be added.
AC_REPLACE_FUNCS(memcpy memmove strdup strcasecmp strerror strncasecmp)

dnl We want to recognize djgpp to avoid the useless warning about no
dnl term library.
AC_CANONICAL_BUILD

# Some GNU/Linux systems (e.g., SuSE 4.3, 1996) don't have curses, but
# rather ncurses.  So we check for it.
TERMLIBS=
# Check for termlib before termcap because Solaris termcap needs libucb.
TERMLIB_VARIANTS="ncurses curses termlib termcap terminfo"
for termlib in ${TERMLIB_VARIANTS}; do
  AC_CHECK_LIB(${termlib}, tgetent,
     [TERMLIBS="${TERMLIBS} -l${termlib}"; break])
done
# don't bother warning on djgpp, it doesn't have a term library, it
# ports each termcap-needing program separately according to its needs.
if test -z "$TERMLIBS" && echo "$build" | grep -v djgpp >/dev/null; then
  AC_MSG_WARN([probably need a terminal library, one of: ${TERMLIB_VARIANTS}])
fi

dnl Checks for variables.
# HP-UX 9 (at least) needs -lncurses which defines termcap variables PC etc.
AC_MSG_CHECKING(for library with termcap variables)
AC_CACHE_VAL(ac_cv_var_ospeed,
oldLIBS=$LIBS 
for trylib in $termlib ${TERMLIB_VARIANTS}; do
   if test "x$trylib" != "x$termlib"; then
     LIBS="$oldLIBS -l$termlib -l$trylib"
   else
     LIBS="$oldLIBS -l$termlib"
   fi
   AC_TRY_LINK(, 
#ifdef HAVE_NCURSES_TERMCAP_H
#include <ncurses/termcap.h>
#else
#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#else
#undef PC
char *BC;
char **UP;
char PC;      
short ospeed; 
#endif
#endif
/* Make sure all variables actually exist.  AIX 4.3 has ospeed but no BC.
   --Andreas Ley <andy@rz.uni-karlsruhe.de>, 24 Aug 2000.  */
BC++;
UP++
PC++;
return ospeed != 0;
, ac_cv_var_ospeed=$trylib; break)
done
LIBS=$oldLIBS
)
AC_MSG_RESULT($ac_cv_var_ospeed)
if test -n "$ac_cv_var_ospeed" \
   && test "x$termlib" != "x$ac_cv_var_ospeed"; then
  TERMLIBS="${TERMLIBS} -l${ac_cv_var_ospeed}"
fi
AC_SUBST(TERMLIBS)dnl

dnl gnulib.
UTILS_FUNC_MKSTEMP

# For teTeX and TeX Live.
AC_CANONICAL_HOST
AC_ARG_ENABLE(multiplatform,
  [  --enable-multiplatform  put executables in bin/PLATFORM])
test "x$enable_multiplatform" = xyes \
&& test "x$bindir" = 'x${exec_prefix}/bin' \
&& bindir="$bindir/$host"

dnl i18n support.  To update to a new version of gettext, run:
dnl  gettextize -f -c --intl
AM_GNU_GETTEXT_VERSION(0.11.5)
AM_GNU_GETTEXT()

AC_CONFIG_FILES([
  Makefile			\
  doc/Makefile			\
  info/Makefile			\
  intl/Makefile			\
  lib/Makefile			\
  m4/Makefile			\
  makeinfo/Makefile		\
  makeinfo/tests/Makefile	\
  po/Makefile.in		\
  util/Makefile			\
])
AC_OUTPUT
