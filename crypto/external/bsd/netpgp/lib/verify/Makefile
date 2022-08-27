# $NetBSD: Makefile,v 1.14 2022/08/27 08:58:32 rillig Exp $

LIB=netpgpverify
SRCS+=b64.c
SRCS+=bignum.c
SRCS+=bufgap.c
SRCS+=digest.c
SRCS+=libverify.c
SRCS+=misc.c
SRCS+=pgpsum.c
SRCS+=rsa.c
SRCS+=bzlib.c
SRCS+=zlib.c
SRCS+=sha1.c
SRCS+=sha2.c
SRCS+=md5c.c
SRCS+=rmd160.c
CPPFLAGS+=-I${.CURDIR} -I${EXTDIST}/src/netpgpverify
MAN=libnetpgpverify.3
WARNS=5

EXTDIST=${.CURDIR}/../../dist
.PATH: ${EXTDIST}/src/netpgpverify

INCS=verify.h
INCSDIR=/usr/include/netpgp

.include <bsd.own.mk>

COPTS+=			${${ACTIVE_CC} == "gcc":? -Wno-implicit-fallthrough :}
COPTS.libverify.c+=	${GCC_NO_FORMAT_TRUNCATION}

LINTFLAGS.bzlib.c+=	-X 161	# constant in conditional context
LINTFLAGS.bzlib.c+=	-X 220	# fallthrough on case statement
LINTFLAGS.zlib.c+=	-X 220	# fallthrough on case statement

LDFLAGS+=-Wl,--version-script=${.CURDIR}/verify.map

.include <bsd.lib.mk>
