#	$NetBSD: Makefile.inc,v 1.2.8.1 2001/11/16 16:20:44 thorpej Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	__isthreaded.c pthread-indr.c pthread-stub.c
