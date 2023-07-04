# $NetBSD: Makefile,v 1.6 2023/07/04 16:23:15 riastradh Exp $

WARNS=	2

.include <bsd.own.mk>

LIB=	bsdmalloc
SRCS=	malloc.c
MAN=	bsdmalloc.3

CFLAGS+=	-fno-builtin-aligned_alloc
CFLAGS+=	-fno-builtin-calloc
CFLAGS+=	-fno-builtin-free
CFLAGS+=	-fno-builtin-malloc
CFLAGS+=	-fno-builtin-posix_memalign
CFLAGS+=	-fno-builtin-realloc

CPPFLAGS+= -D_REENT -D_REENTRANT -I${.CURDIR}/../libc/include/

.include <bsd.lib.mk>
