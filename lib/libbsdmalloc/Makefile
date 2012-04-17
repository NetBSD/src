# $NetBSD: Makefile,v 1.3.4.1 2012/04/17 00:05:12 yamt Exp $

NOMAN=1

WARNS=	2

.include <bsd.own.mk>

LIB=	bsdmalloc
SRCS=	malloc.c

CPPFLAGS+= -D_REENT -D_REENTRANT -I${.CURDIR}/../libc/include/

.include <bsd.lib.mk>
