#	$NetBSD: Makefile,v 1.7 2002/09/18 03:54:39 lukem Exp $

.include <bsd.own.mk>

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
