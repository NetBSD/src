#	$NetBSD: Makefile.inc,v 1.5 2026/10/04 03:28:46 mrg Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.PARSEDIR}

SRCS+=	__isthreaded.c thread-stub.c thread-stub-init.c
