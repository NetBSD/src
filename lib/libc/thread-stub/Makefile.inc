#	$NetBSD: Makefile.inc,v 1.1 1999/11/14 18:34:15 explorer Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread-stub

SRCS+=	pthread-stub.c
