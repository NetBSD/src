#	$NetBSD: Makefile,v 1.2 2016/11/01 21:56:12 christos Exp $

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
