#	$NetBSD: Makefile,v 1.1 2014/07/08 05:55:33 spz Exp $

.include <bsd.own.mk>

PROG=	delv
SRCS=	delv.c
BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/delv
CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include
LDADD+=    -lirs

.PATH: ${DIST}

.include <bsd.prog.mk>
