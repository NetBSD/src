#	$NetBSD: Makefile,v 1.6 2002/01/05 07:37:45 mrg Exp $

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${.CURDIR}/../../lib/libc/time

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
