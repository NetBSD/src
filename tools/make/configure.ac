#	$NetBSD: configure.ac,v 1.1 2002/01/15 19:15:47 tv Exp $
#
# Autoconf definition file for make.
#

AC_INIT([make], [noversion], [bin-bug-people@netbsd.org])
AC_CONFIG_FILES(buildmake.sh)

# Make sure we have POSIX regex ability.
AC_CHECK_HEADER(regex.h,, AC_MSG_ERROR([POSIX regex.h is required]))

# regcomp() and regexec() are also names of functions in the old V8
# regexp package.  To avoid them, we need to find out who has regfree().

dnl # Cygwin: We *MUST* look at -lregex *before* the "no libs" condition.
dnl # Thus AC_CHECK_LIB(regex...) comes first, and AC_SEARCHLIBS next.
AC_CHECK_LIB(regex, regfree)
AC_SEARCH_LIBS(regfree, rx posix)

AC_OUTPUT
