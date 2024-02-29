#	$NetBSD: Makefile,v 1.1.6.1 2024/02/29 12:28:00 martin Exp $

.include <bsd.init.mk>

PROG=	host
SRCS=	host.c dighost.c
BINDIR=	/usr/bin

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
