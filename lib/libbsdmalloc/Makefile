# $NetBSD: Makefile,v 1.7 2023/07/04 18:40:14 riastradh Exp $

WARNS=	3

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
