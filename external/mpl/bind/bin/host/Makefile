#	$NetBSD: Makefile,v 1.3 2024/02/21 22:50:56 christos Exp $

.include <bsd.init.mk>

PROG=	host
SRCS=	host.c dighost.c
BINDIR=	/usr/bin

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
