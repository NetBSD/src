#	$NetBSD: configure.ac,v 1.15 2002/04/18 15:31:52 bjh21 Exp $
#
# Autoconf definition file for libnbcompat.
#

AC_INIT([libnbcompat], [noversion], [lib-bug-people@netbsd.org])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(defs.mk)

AC_C_BIGENDIAN
AC_HEADER_STDC

# Confirm existence of zlib.  (This is available as a default install
# option on many OS's; this could be added as a reachover build in the
# future.)
AC_CHECK_HEADER(zlib.h,,
	AC_MSG_ERROR([zlib must be installed in a compiler-visible path]))
AC_CHECK_LIB(z, gzdopen,,
	AC_MSG_ERROR([zlib must be installed in a compiler-visible path]))

# Make sure certain required headers are available.
# These are not necessarily required by the code, but they are not
# currently conditionalized.
AC_CHECK_HEADERS(sys/ioctl.h sys/mman.h sys/mtio.h sys/param.h \
	sys/stat.h sys/time.h sys/types.h sys/utsname.h sys/wait.h \
	assert.h ctype.h errno.h fcntl.h grp.h limits.h locale.h \
	netdb.h pwd.h signal.h stdarg.h stdio.h stdlib.h string.h \
	unistd.h,,
	AC_MSG_ERROR([standard system header file not found]))

# Find headers that may not be available.
AC_HEADER_DIRENT
AC_CHECK_HEADERS(sys/sysmacros.h sys/syslimits.h features.h malloc.h stddef.h stdint.h)
AC_CHECK_HEADERS(machine/bswap.h sys/cdefs.h sys/featuretest.h \
	err.h inttypes.h libgen.h paths.h util.h,,
	[test -f include/$ac_header || touch include/$ac_header])
AC_CHECK_HEADERS(rpc/types.h fts.h getopt.h md2.h md4.h md5.h netconfig.h \
	rmd160.h sha1.h vis.h,,
	[echo '#include "config.h"' >include/$ac_header.new
	echo '#include "'$srcdir/../../include/$ac_header'"' >>include/$ac_header.new
	if cmp include/$ac_header.new include/$ac_header >/dev/null 2>&1; then
		rm -f include/$ac_header.new
	else
		mv -f include/$ac_header.new include/$ac_header
	fi])

# Typedefs.
AC_TYPE_SIZE_T
AC_CHECK_TYPES([id_t, long long, u_quad_t])

dnl XXX - This is UGLY.  Need a better way to homogenize the bitsized types,
dnl including use of compiler primitive types via AC_CHECK_SIZEOF.
dnl
define([NB_CHECK_INTTYPE], [
	AC_CHECK_TYPE(uint][$1][_t,, [
		AC_CHECK_TYPE(u_int][$1][_t,
			AC_DEFINE(uint][$1][_t, u_int][$1][_t),
			AC_MSG_ERROR([cannot find a suitable type for uint][$1][_t]))
	])
	AC_CHECK_TYPE(u_int][$1][_t,, [
		AC_CHECK_TYPE(uint][$1][_t,
			AC_DEFINE(u_int][$1][_t, uint][$1][_t),
			AC_MSG_ERROR([cannot find a suitable type for u_int][$1][_t]))
	])
])

NB_CHECK_INTTYPE(8)
NB_CHECK_INTTYPE(16)
NB_CHECK_INTTYPE(32)
NB_CHECK_INTTYPE(64)

# Struct members.
AC_CHECK_MEMBERS([DIR.dd_fd, struct dirent.d_namlen],,,
	[#include <dirent.h>])
AC_CHECK_MEMBERS([struct stat.st_flags, struct stat.st_gen,
	struct stat.st_mtimensec],,, [#include <sys/stat.h>])
AC_CHECK_MEMBERS(struct statfs.f_iosize,,, [#include <sys/mount.h>])

# Global variable decls.
AC_CHECK_DECLS([optind, optreset],,, [
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
])
AC_CHECK_DECLS(sys_signame,,, [#include <signal.h>])

# Library functions (where a .h check isn't enough).
AC_SEARCH_LIBS(fparseln, util)
AC_CHECK_FUNCS(asprintf asnprintf basename dirfd dirname \
	fgetln flock fparseln futimes getopt getopt_long \
	isblank issetugid lchmod lchown lutimes pread pwcache_userdb \
	pwrite setenv setgroupent setprogname setpassent \
	snprintf strlcat strlcpy strsep vasprintf vasnprintf vsnprintf)

AC_CHECK_DECLS([setgroupent, setpassent],,, [
#include <grp.h>
#include <pwd.h>
])

# regcomp() and regexec() are also names of functions in the old V8
# regexp package.  To avoid them, we need to find out who has regfree().

dnl # Cygwin: We *MUST* look at -lregex *before* the "no libs" condition.
dnl # Thus AC_CHECK_LIB(regex...) comes first, and AC_SEARCHLIBS next.
AC_CHECK_LIB(regex, regfree)
AC_SEARCH_LIBS(regfree, rx posix)

AC_OUTPUT
