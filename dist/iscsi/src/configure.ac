dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.57)
AC_INIT([netbsd-iscsi],[20060526],[Alistair Crooks <agc@NetBSD.org>])
AC_CONFIG_SRCDIR([iscsi.c])
AC_CONFIG_HEADER(../include/config.h)

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_RANLIB

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/types.h sys/param.h sys/stat.h sys/time.h sys/mman.h sys/uio.h sys/socket.h sys/time.h sys/vfs.h)
AC_CHECK_HEADERS(arpa/inet.h netinet/in.h netinet/tcp.h netdb.h)
AC_CHECK_HEADERS(asm/byteorder.h sys/bswap.h sys/byteorder.h libkern/OSByteOrder.h byteswap.h machine/endian.h)
AC_CHECK_HEADERS(ctype.h errno.h fcntl.h pthread.h pwd.h signal.h stdlib.h syslog.h unistd.h string.h stdarg.h utime.h uuid.h)

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

dnl check for libraries
AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(socket, connect)
AC_CHECK_LIB(resolv, inet_aton)

dnl Check for functionality
AC_CHECK_FUNCS(__bswap64 asprintf asnprintf bswap64 daemon htobe64 fsync_range snprintf strlcpy strtoll syslog uuid_create uuid_to_string vasprintf vasnprintf vsnprintf)

dnl that's it for now...
AC_OUTPUT(Makefile)
