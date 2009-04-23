dnl $Id: configure.ac,v 1.1.1.1 2009/04/23 06:31:56 agc Exp $
dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.57)
AC_INIT([netpgp],[20090422],[Alistair Crooks <agc@netbsd.org> c0596823])
AC_CONFIG_SRCDIR([src/bin/netpgp.c])
AC_CONFIG_HEADER(src/lib/config.h)
AC_ARG_PROGRAM

AC_CANONICAL_HOST
CANONICAL_HOST=$host
AC_SUBST(CANONICAL_HOST)

# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

AUTOCONF=${AUTOCONF-"$srcdir/missing --run autoconf"}
AC_SUBST(AUTOCONF)
AUTOHEADER=${AUTOHEADER-"$srcdir/missing --run autoheader"}
AC_SUBST(AUTOHEADER)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([CommonCrypto/CommonDigest.h])
AC_CHECK_HEADERS([assert.h bzlib.h dmalloc.h direct.h errno.h fcntl.h])
AC_CHECK_HEADERS([inttypes.h libgen.h limits.h malloc.h])
AC_CHECK_HEADERS([stdbool.h zlib.h])
AC_CHECK_HEADERS([openssl/aes.h openssl/bn.h openssl/cast.h openssl/des.h openssl/dsa.h])
AC_CHECK_HEADERS([openssl/err.h openssl/idea.h openssl/md5.h openssl/rand.h openssl/rsa.h openssl/sha.h])
AC_CHECK_HEADERS([sys/cdefs.h sys/file.h sys/mman.h sys/param.h sys/uio.h])

AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_C_BIGENDIAN

# Checks for library functions.

# write it out
AC_CONFIG_FILES([Makefile src/lib/Makefile src/bin/Makefile])
AC_OUTPUT
