#	$NetBSD: Makefile.inc,v 1.2.8.2 2002/05/02 16:56:33 nathanw Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	__isthreaded.c pthread-stub.c
