#	$NetBSD: configure.ac,v 1.1 2001/12/11 21:17:58 tv Exp $
#
# Autoconf definition file for libnbcompat.
#

AC_INIT([libnbcompat], [noversion], [lib-bug-people@netbsd.org])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(defs.mk)

AC_TYPE_SIZE_T

AC_CHECK_HEADER(stddef.h)
AC_CHECK_HEADER(sys/types.h)

AC_CHECK_FUNCS(errx)
AC_SEARCH_LIBS(fparseln, util, AC_DEFINE(HAVE_FPARSELN))
AC_CHECK_FUNCS(setprogname)

AC_OUTPUT
