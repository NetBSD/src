#	$NetBSD: configure.ac,v 1.61 2007/07/03 12:11:10 nakayama Exp $
#
# Autoconf definition file for libnbcompat.
#

AC_INIT([libnbcompat], [noversion], [lib-bug-people@NetBSD.org])
AC_CONFIG_HEADERS(nbtool_config.h)
AC_CONFIG_FILES(defs.mk)

# Autoheader header and footer
AH_TOP([/*      $NetBSD: configure.ac,v 1.61 2007/07/03 12:11:10 nakayama Exp $    */
 
#ifndef __NETBSD_NBTOOL_CONFIG_H__
#define __NETBSD_NBTOOL_CONFIG_H__])

AH_BOTTOM([#include "compat_defs.h"
#endif /* !__NETBSD_NBTOOL_CONFIG_H__ */])

AC_DEFUN([AC_NETBSD],
[AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
      AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
      AC_MSG_CHECKING([for NetBSD])
      AC_EGREP_CPP(yes,
      [#ifdef __NetBSD__
	yes
	#endif
	],
	[AC_MSG_RESULT([yes])
	 AC_DEFINE([_POSIX_SOURCE], 1, [Define for NetBSD headers.])
	 AC_DEFINE([_POSIX_C_SOURCE], 200112L, [Define for NetBSD headers.])
	 AC_DEFINE([_XOPEN_SOURCE], 600, [Define for NetBSD headers.])],
	[AC_MSG_RESULT([no])])
])# AC_NETBSD

AC_NETBSD
AC_PATH_PROG(BSHELL, sh, )
if test x"$BSHELL" = x; then
	AC_MSG_ERROR([sh must be somewhere on \$PATH])
fi
AC_DEFINE_UNQUOTED([PATH_BSHELL], "$BSHELL", [Path to sh(1).])

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
	sys/socket.h sys/stat.h sys/time.h sys/types.h sys/utsname.h \
	sys/wait.h assert.h ctype.h errno.h fcntl.h grp.h limits.h locale.h \
	netdb.h pwd.h signal.h stdarg.h stdio.h stdlib.h string.h \
	termios.h unistd.h,,
	AC_MSG_ERROR([standard system header file not found]))

# Find headers that may not be available.
AC_HEADER_DIRENT
AC_CHECK_HEADERS(sys/sysmacros.h sys/syslimits.h \
	getopt.h features.h malloc.h sys/poll.h stddef.h)
AC_CHECK_HEADERS(sys/bswap.h machine/bswap.h sys/cdefs.h machine/endian.h \
	sys/featuretest.h err.h inttypes.h libgen.h paths.h stdint.h util.h,,
	[test -f include/$ac_header || touch include/$ac_header])
AC_CHECK_HEADERS(rpc/types.h netconfig.h,,
	[echo '#include "nbtool_config.h"' >include/$ac_header.new
	echo '#include "'$srcdir/../../include/$ac_header'"' >>include/$ac_header.new
	if cmp include/$ac_header.new include/$ac_header >/dev/null 2>&1; then
		rm -f include/$ac_header.new
	else
		mv -f include/$ac_header.new include/$ac_header
	fi])

# Typedefs.
AC_TYPE_SIZE_T
AC_CHECK_TYPES([id_t, long long, u_long, u_char, u_short, u_int, u_quad_t])
AC_CHECK_TYPE(socklen_t, [AC_DEFINE([HAVE_SOCKLEN_T], 1,
			  [Define if you have the socklen_t type.])],,
[#include <sys/types.h>
#include <sys/socket.h>])

dnl XXX - This is UGLY.  Need a better way to homogenize the bitsized types,
dnl including use of compiler primitive types via AC_CHECK_SIZEOF.
dnl
define([NB_CHECK_INTTYPE], [
	AC_CHECK_TYPE(uint][$1][_t,, [
		AC_CHECK_TYPE(u_int][$1][_t,
			AC_DEFINE(uint][$1][_t, u_int][$1][_t, \
			          [Define if you have u_int][$1][_t, but not uint][$1][_t.]),
			AC_MSG_ERROR([cannot find a suitable type for uint][$1][_t]))
	])
	AC_CHECK_TYPE(u_int][$1][_t,, [
		AC_CHECK_TYPE(uint][$1][_t,
			AC_DEFINE(u_int][$1][_t, uint][$1][_t, \
				  [Define if you have uint][$1][_t, but not u_int][$1][_t.]),
			AC_MSG_ERROR([cannot find a suitable type for u_int][$1][_t]))
	])
])

NB_CHECK_INTTYPE(8)
NB_CHECK_INTTYPE(16)
NB_CHECK_INTTYPE(32)
NB_CHECK_INTTYPE(64)

# Struct members.
AC_CHECK_MEMBERS([DIR.dd_fd, DIR.__dd_fd, struct dirent.d_namlen],,,
[#include <sys/types.h>
#include <dirent.h>])
AC_CHECK_MEMBERS([struct stat.st_flags, struct stat.st_gen,
	struct stat.st_birthtime, struct stat.st_birthtimensec,
	struct stat.st_atim, struct stat.st_mtimensec],,, 
	[#include <sys/stat.h>])
AC_CHECK_MEMBERS(struct statvfs.f_iosize,,, [#include <sys/statvfs.h>])

# Global variable decls.
AC_CHECK_DECLS([optind, optreset],,, [
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
])
AC_CHECK_DECLS(sys_signame,,, [#include <signal.h>])

# Library functions (where a .h check isn't enough).
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(atoll asprintf asnprintf basename devname dirfd dirname \
	esetfunc fgetln flock fparseln futimes getopt getopt_long \
	group_from_gid \
	heapsort isblank issetugid lchflags lchmod lchown lutimes mkstemp \
	mkdtemp poll pread putc_unlocked pwcache_userdb pwrite random setenv \
	setgroupent setprogname setpassent snprintf strlcat strlcpy strmode \
	strndup strsep strsuftoll strtoll \
	user_from_uid vasprintf vasnprintf vsnprintf)

AC_CHECK_DECLS([strsuftoll],,,[#include <stdlib.h>])

AC_CHECK_DECLS([htobe16, htobe32, htobe64, htole16, htole32, htole64, be16toh, be32toh, be64toh, le16toh, le32toh, le64toh],,, [#include <sys/types.h>])

AC_CHECK_DECLS([bswap16, bswap32, bswap64],,, [#include <machine/bswap.h>])

AC_CHECK_DECLS([fstatvfs],,, [#include <sys/statvfs.h>])

AC_CHECK_DECLS([setgroupent, setpassent],,, [
#include <sys/types.h>
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
