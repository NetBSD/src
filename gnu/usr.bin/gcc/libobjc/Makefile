#	$NetBSD: Makefile,v 1.7 1998/02/19 02:17:06 cgd Exp $

LIB=	objc
INCSDIR=/usr/include/objc
INCS=	NXConstStr.h Object.h Protocol.h \
	encoding.h hash.h list.h objc.h objc-api.h sarray.h typedstream.h
NOLINT=		nolint (gcc private fns library)
NOPROFILE=
NOPIC=

CPPFLAGS+=-I${.CURDIR}/../common -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/${MACHINE_ARCH}


SRCS=	NXConstStr.m Object.m Protocol.m archive.c class.c \
	encoding.c hash.c init.c misc.c objects.c \
	sarray.c selector.c sendmsg.c 

.include <bsd.lib.mk>
