# $NetBSD: Makefile,v 1.9.12.1 2019/06/10 21:41:10 christos Exp $

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

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.lib.mk>
