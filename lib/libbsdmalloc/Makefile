# $NetBSD: Makefile,v 1.2 2003/07/10 11:27:45 lukem Exp $

NOMAN=1

.include <bsd.own.mk>

LIB=	bsdmalloc
SRCS=	malloc.c

CPPFLAGS= -D_REENTRANT
CPPFLAGS= -I${.CURDIR}/../libc/include/

.include <bsd.lib.mk>
