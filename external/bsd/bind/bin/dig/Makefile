#	$NetBSD: Makefile,v 1.1.30.1 2016/11/05 17:47:30 martin Exp $

.include <bsd.own.mk>

PROG=	dig
SRCS=	dig.c dighost.c
BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

dig.o: ${VERSIONFILE}

.PATH: ${DIST}

.include <bsd.prog.mk>
