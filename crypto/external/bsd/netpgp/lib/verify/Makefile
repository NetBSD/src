# $NetBSD: Makefile,v 1.3 2012/11/22 04:05:57 agc Exp $

.include <bsd.own.mk>

LIB=netpgpverify
SRCS=libverify.c b64.c pgpsum.c
SRCS+=digest.c tiger.c
SRCS+=bignum.c misc.c
CPPFLAGS+=-I${EXTDIST}/src/libbn
CPPFLAGS+=-I${EXTDIST}/src/libdigest
CPPFLAGS+=-I${EXTDIST}/src/librsa
MAN=libnetpgpverify.3
WARNS=5

EXTDIST=${.CURDIR}/../../dist
.PATH: ${EXTDIST}/src/libverify ${EXTDIST}/src/libdigest ${EXTDIST}/src/libbn ${EXTDIST}/src/librsa

LIBDPLIBS+=     z               ${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=     bz2             ${NETBSDSRCDIR}/lib/libbz2

INCS=verify.h
INCSDIR=/usr/include/netpgp

.include <bsd.lib.mk>
