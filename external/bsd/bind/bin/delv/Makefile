#	$NetBSD: Makefile,v 1.2.12.1 2017/01/18 08:45:49 skrll Exp $

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
