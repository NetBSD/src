#	$NetBSD: Makefile,v 1.1.32.1 2016/11/04 14:42:41 pgoyette Exp $

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
