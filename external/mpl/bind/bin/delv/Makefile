#	$NetBSD: Makefile,v 1.1 2018/08/12 13:02:24 christos Exp $

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
