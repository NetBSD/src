#	$NetBSD: Makefile,v 1.1.28.1 2016/11/05 17:54:06 martin Exp $

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
