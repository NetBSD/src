#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 22:02:53 christos Exp $

.include <bsd.own.mk>

PROG=	delv
SRCS=	delv.c
BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/delv
CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include
LDADD+=    -lirs -lisccfg

delv.o: ${VERSIONFILE}

.PATH: ${DIST}

.include <bsd.prog.mk>
