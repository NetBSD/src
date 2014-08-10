#	$NetBSD: Makefile,v 1.2.2.2 2014/08/10 07:06:33 tls Exp $

.include <bsd.own.mk>

PROG=	delv
SRCS=	delv.c
BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/delv
CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include
LDADD+=    -lirs -lisccfg

.PATH: ${DIST}

.include <bsd.prog.mk>
