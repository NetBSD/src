#	$NetBSD: configure.ac,v 1.2 2002/09/15 17:23:01 thorpej Exp $
#
# Autoconf definition file for mkdep.
#

AC_INIT([mkdep], [noversion], [tech-toolchain@netbsd.org])

AC_PATH_PROG(BSHELL, sh, )
if test x"$BSHELL" = x; then
	AC_MSG_ERROR([sh must be somewhere on \$PATH])
fi

AC_CONFIG_FILES(host-mkdep)

AC_PROG_AWK
AC_PROG_CPP

AC_OUTPUT
