#	$NetBSD: Makefile.inc,v 1.4.40.1 2026/11/04 23:19:24 perseant Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.PARSEDIR}

SRCS+=	__isthreaded.c thread-stub.c thread-stub-init.c
