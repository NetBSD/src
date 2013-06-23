#	$NetBSD: Makefile.inc,v 1.3.62.1 2013/06/23 06:21:06 tls Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	__isthreaded.c thread-stub.c thread-stub-init.c
