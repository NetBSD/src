#	$NetBSD: configure.ac,v 1.3 2002/01/10 04:31:15 lukem Exp $
#
# Autoconf definition file for libnbcompat.
#

AC_INIT([libnbcompat], [noversion], [lib-bug-people@netbsd.org])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(defs.mk)

AC_TYPE_SIZE_T

AC_CHECK_HEADER(sys/types.h stddef.h inttypes.h)

AC_CHECK_FUNCS(errx fgetln setprogname)
AC_SEARCH_LIBS(fparseln, util, AC_DEFINE(HAVE_FPARSELN))

AC_OUTPUT
