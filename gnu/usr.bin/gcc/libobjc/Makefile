#	$NetBSD: Makefile,v 1.1.1.2 1997/05/14 05:08:29 phil Exp $

LIB=	objc
NOPROFILE=
NOPIC=

CFLAGS+=-I${.CURDIR}/../common -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/${MACHINE_ARCH}

SRCS=	archive.c class.c encoding.c hash.c init.c \
	misc.c objects.c sarray.c selector.c sendmsg.c \
	
.include <bsd.lib.mk>
