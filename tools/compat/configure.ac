#	$NetBSD: configure.ac,v 1.4 2002/01/21 20:04:36 tv Exp $
#
# Autoconf definition file for libnbcompat.
#

AC_INIT([libnbcompat], [noversion], [lib-bug-people@netbsd.org])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(defs.mk)

AC_C_BIGENDIAN

# Make sure certain required headers are available.
# These are not necessarily required by the code, but they are not
# currently conditionalized.
AC_CHECK_HEADERS(sys/cdefs.h sys/mman.h sys/param.h sys/stat.h \
	sys/types.h sys/utsname.h sys/wait.h \
	ctype.h errno.h fcntl.h grp.h limits.h locale.h paths.h pwd.h \
	signal.h stdio.h stdlib.h string.h unistd.h,,
	AC_MSG_ERROR([standard system header file not found]))

# Find headers that may not be available.
AC_CHECK_HEADERS(err.h inttypes.h libgen.h stddef.h vis.h)

# Typedefs.
AC_TYPE_SIZE_T
AC_CHECK_TYPES(id_t)

# Struct members.
AC_CHECK_MEMBERS(struct stat.st_flags,,, [#include <sys/stat.h>])

# Global variable decls.
AC_CHECK_DECLS(sys_signame,,, [#include <signal.h>])

# Library functions (where a .h check isn't enough).
AC_SEARCH_LIBS(fparseln, util)
AC_CHECK_FUNCS(basename dirname fgetln flock fparseln futimes pread pwrite setprogname strlcat strlcpy)

# regcomp() and regexec() are also names of functions in the old V8
# regexp package.  To avoid them, we need to find out who has regfree().

dnl # Cygwin: We *MUST* look at -lregex *before* the "no libs" condition.
dnl # Thus AC_CHECK_LIB(regex...) comes first, and AC_SEARCHLIBS next.
AC_CHECK_LIB(regex, regfree)
AC_SEARCH_LIBS(regfree, rx posix)

AC_OUTPUT
