#	$NetBSD: Makefile,v 1.3 1997/10/22 12:57:07 is Exp $

LIB=	objc
INCSDIR=/usr/include/objc
INCS=	NXConstStr.h Object.h Protocol.h \
	encoding.h hash.h list.h objc.h objc-api.h sarray.h typedstream.h
NOPROFILE=
NOPIC=

.SUFFIXES: .o .m

.m.o: ;	${CC} -c ${CFLAGS} -o ${.TARGET} ${.IMPSRC}

CFLAGS+=-I${.CURDIR}/../common -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/${MACHINE_ARCH}


SRCS=	NXConstStr.m Object.m Protocol.m archive.c class.c \
	encoding.c hash.c init.c misc.c objects.c \
	sarray.c selector.c sendmsg.c 

.include <bsd.lib.mk>
