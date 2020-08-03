#	$NetBSD: Makefile,v 1.2 2020/08/03 17:23:35 christos Exp $

.include <bsd.init.mk>

PROG=	host
SRCS=	host.c dighost.c
BINDIR=	/usr/bin

DIST=${IDIST}/bin/dig
CPPFLAGS+= -I${DIST}/include

host.o: ${VERSIONFILE}

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
