# $NetBSD: Makefile,v 1.1 2003/04/21 22:21:06 elric Exp $

.include <bsd.own.mk>

LIB=	bsdmalloc
SRCS=	malloc.c

CPPFLAGS= -D_REENTRANT
CPPFLAGS= -I${.CURDIR}/../libc/include/

NOMAN=1

.include <bsd.lib.mk>
