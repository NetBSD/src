#	$NetBSD: Makefile,v 1.2 1998/04/03 02:20:31 tv Exp $

LIB=	objc
INCSDIR=/usr/include/objc
INCS=	NXConstStr.h Object.h Protocol.h \
	encoding.h hash.h objc.h objc-api.h sarray.h typedstream.h
NOLINT=		nolint (gcc private fns library)
NOPROFILE=
NOPIC=

SRCS=	NXConstStr.m Object.m Protocol.m archive.c class.c \
	encoding.c hash.c init.c misc.c objects.c \
	sarray.c selector.c sendmsg.c 

.include <bsd.lib.mk>

.PATH: ${DIST}/objc
