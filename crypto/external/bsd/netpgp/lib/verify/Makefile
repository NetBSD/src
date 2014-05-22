# $NetBSD: Makefile,v 1.5.2.4 2014/05/22 13:21:34 yamt Exp $

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

INCS=verify.h
INCSDIR=/usr/include/netpgp

LIBDPLIBS+=	z		${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2		${NETBSDSRCDIR}/lib/libbz2

.include <bsd.lib.mk>

.if ${HAVE_GCC:U} == "45" && ${MACHINE_CPU} == "ia64"
COPTS.bignum.c+=	${${ACTIVE_CC} == "gcc" :? -O1 :}
.endif

