#	$NetBSD: Makefile.inc,v 1.2 1999/12/03 06:37:34 explorer Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	__isthreaded.c pthread-stub.c
