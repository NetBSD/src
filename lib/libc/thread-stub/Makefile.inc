#	$NetBSD: Makefile.inc,v 1.3.56.1 2014/05/22 11:36:54 yamt Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	__isthreaded.c thread-stub.c thread-stub-init.c
