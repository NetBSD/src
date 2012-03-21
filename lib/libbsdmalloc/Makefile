# $NetBSD: Makefile,v 1.4 2012/03/21 10:08:30 matt Exp $

NOMAN=1

WARNS=	2

.include <bsd.own.mk>

LIB=	bsdmalloc
SRCS=	malloc.c

CPPFLAGS+= -D_REENT -D_REENTRANT -I${.CURDIR}/../libc/include/

.include <bsd.lib.mk>
