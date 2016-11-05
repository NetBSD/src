#	$NetBSD: Makefile,v 1.2.4.1 2016/11/05 17:54:05 martin Exp $

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
