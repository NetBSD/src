#	$NetBSD: configure.ac,v 1.3 2002/04/24 21:20:08 bjh21 Exp $
#
# Autoconf definition file for make.
#

AC_INIT([make], [noversion], [bin-bug-people@netbsd.org])
AC_CONFIG_FILES(buildmake.sh)

AC_PATH_PROG(BSHELL, sh)
if test x"$BSHELL" = x; then
	AC_MSG_ERROR([sh must be somewhere on \$PATH])
fi
# Make wants to know what directory to find sh in.
nb_path_defshelldir=`echo $BSHELL | sed 's,/sh$,,'`
AC_DEFINE_UNQUOTED(_PATH_DEFSHELLDIR, "$nb_path_defshelldir")

# Make sure we have POSIX regex ability.
AC_CHECK_HEADER(regex.h,, AC_MSG_ERROR([POSIX regex.h is required]))

# regcomp() and regexec() are also names of functions in the old V8
# regexp package.  To avoid them, we need to find out who has regfree().

dnl # Cygwin: We *MUST* look at -lregex *before* the "no libs" condition.
dnl # Thus AC_CHECK_LIB(regex...) comes first, and AC_SEARCHLIBS next.
AC_CHECK_LIB(regex, regfree)
AC_SEARCH_LIBS(regfree, rx posix)

AC_CHECK_FUNCS(setenv strdup strerror strftime vsnprintf)

AC_OUTPUT
