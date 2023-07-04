# $NetBSD: Makefile,v 1.5 2023/07/04 15:08:55 riastradh Exp $

NOMAN=1

WARNS=	2

.include <bsd.own.mk>

LIB=	bsdmalloc
SRCS=	malloc.c

CFLAGS+=	-fno-builtin-aligned_alloc
CFLAGS+=	-fno-builtin-calloc
CFLAGS+=	-fno-builtin-free
CFLAGS+=	-fno-builtin-malloc
CFLAGS+=	-fno-builtin-posix_memalign
CFLAGS+=	-fno-builtin-realloc

CPPFLAGS+= -D_REENT -D_REENTRANT -I${.CURDIR}/../libc/include/

.include <bsd.lib.mk>
