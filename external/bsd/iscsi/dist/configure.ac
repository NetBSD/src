dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.57)
AC_INIT([netbsd-iscsi],[20090622],[Alistair Crooks <agc@NetBSD.org>])

AC_CONFIG_SRCDIR([src/lib/iscsi.c])
AC_CONFIG_AUX_DIR([buildaux])
AC_CONFIG_HEADER(include/config.h)

AS_SHELL_SANITIZE

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_MAINTAINER_MODE

AC_CANONICAL_HOST
CANONICAL_HOST=$host
AC_SUBST([CANONICAL_HOST])

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/types.h sys/param.h sys/stat.h sys/time.h)
AC_CHECK_HEADERS(sys/mman.h sys/uio.h sys/socket.h sys/time.h sys/vfs.h)
AC_CHECK_HEADERS(arpa/inet.h netinet/in.h netinet/tcp.h netdb.h poll.h)
AC_CHECK_HEADERS(asm/byteorder.h sys/bswap.h sys/byteorder.h sys/select.h)
AC_CHECK_HEADERS(libkern/OSByteOrder.h byteswap.h machine/endian.h)
AC_CHECK_HEADERS(ctype.h errno.h fcntl.h pthread.h pwd.h signal.h stdint.h)
AC_CHECK_HEADERS(stdlib.h syslog.h unistd.h string.h stdarg.h utime.h uuid.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES(socklen_t,,,[
#include <sys/socket.h>
#include <arpa/inet.h>
])
AC_CHECK_TYPES([long long, unsigned long long])
AC_DEFINE_UNQUOTED(HAVE_SOCKLEN_T, $ac_cv_type_socklen_t)
AC_DEFINE_UNQUOTED(HAVE_LONG_LONG, $ac_cv_type_long_long)
AC_DEFINE_UNQUOTED(HAVE_UNSIGNED_LONG_LONG, $ac_cv_type_unsigned_long_long)

# Checks for typedefs, structures, and compiler characteristics.
#
AC_TYPE_UNSIGNED_LONG_LONG_INT
AC_TYPE_LONG_LONG_INT
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

dnl check for libraries
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([connect], [socket])
AC_SEARCH_LIBS([inet_aton], [resolv])
AC_SEARCH_LIBS([pthread_create], [pthread])

dnl Check for functionality
AC_CHECK_FUNCS(__bswap64 asprintf asnprintf bswap64 daemon fsync_range)
AC_CHECK_FUNCS(getaddrinfo getnameinfo htobe64 memset bcopy poll snprintf)
AC_CHECK_FUNCS(strlcpy strtoll syslog uuid_create uuid_to_string vasprintf)
AC_CHECK_FUNCS(vasprintf vasnprintf vsnprintf)

dnl that's it for now...
AC_OUTPUT(Makefile)
AC_OUTPUT(src/Makefile)
AC_OUTPUT(src/lib/Makefile)
AC_OUTPUT(src/target/Makefile)
AC_OUTPUT(src/initiator/Makefile)
