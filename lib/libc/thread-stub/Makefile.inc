#	$NetBSD: Makefile.inc,v 1.2.2.1 1999/12/27 18:29:54 wrstuden Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	__isthreaded.c pthread-stub.c
