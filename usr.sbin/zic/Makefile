#	$NetBSD: Makefile,v 1.5 1998/09/01 03:16:49 enami Exp $

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${.CURDIR}/../../lib/libc/time
.include <bsd.prog.mk>
