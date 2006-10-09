#	$NetBSD: configure.ac,v 1.7 2006/10/09 20:51:05 apb Exp $
#
# Autoconf definition file for make.
#

AC_INIT([make], [noversion], [bin-bug-people@NetBSD.org])
AC_CONFIG_FILES(buildmake.sh)

AC_PATH_PROG(BSHELL, sh)
if test x"$BSHELL" = x; then
	AC_MSG_ERROR([sh must be somewhere on \$PATH])
fi
# Make wants to get the directory name and the basename
# for the default shell in two separate variables.
nb_path_defshelldir="${BSHELL%/*}"
nb_basename_defshell="${BSHELL##*/}"
AC_DEFINE_UNQUOTED(_PATH_DEFSHELLDIR, "$nb_path_defshelldir")
AC_DEFINE_UNQUOTED(_BASENAME_DEFSHELL, "$nb_basename_defshell")

# Make sure we have POSIX regex ability.
AC_CHECK_HEADER(regex.h,, AC_MSG_ERROR([POSIX regex.h is required]))

# If we don't have <poll.h>, we need to use select(2).
AC_CHECK_HEADER(poll.h,, AC_DEFINE(USE_SELECT))

# regcomp() and regexec() are also names of functions in the old V8
# regexp package.  To avoid them, we need to find out who has regfree().

dnl # Cygwin: We *MUST* look at -lregex *before* the "no libs" condition.
dnl # Thus AC_CHECK_LIB(regex...) comes first, and AC_SEARCHLIBS next.
AC_CHECK_LIB(regex, regfree)
AC_SEARCH_LIBS(regfree, rx posix)

AC_CHECK_FUNCS(setenv strdup strerror strftime vsnprintf)

AC_OUTPUT
