#	$NetBSD: Makefile,v 1.2.6.1 2024/02/25 15:42:52 martin Exp $

.include <bsd.init.mk>

PROG=	dig
SRCS=	dig.c dighost.c
BINDIR=	/usr/bin

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
