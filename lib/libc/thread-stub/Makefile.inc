#	$NetBSD: Makefile.inc,v 1.2.8.3 2003/01/08 19:34:20 thorpej Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	__isthreaded.c thread-stub.c
