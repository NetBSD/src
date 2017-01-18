#	$NetBSD: Makefile,v 1.1.34.1 2017/01/18 08:45:50 skrll Exp $

.include <bsd.own.mk>

PROG=	host
SRCS=	host.c dighost.c
BINDIR=	/usr/bin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

host.o: ${VERSIONFILE}

.PATH: ${DIST}

.include <bsd.prog.mk>
