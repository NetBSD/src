#	$NetBSD: Makefile,v 1.4 1997/10/25 10:11:15 lukem Exp $

LIB=	objc
INCSDIR=/usr/include/objc
INCS=	NXConstStr.h Object.h Protocol.h \
	encoding.h hash.h list.h objc.h objc-api.h sarray.h typedstream.h
NOPROFILE=
NOPIC=

.SUFFIXES: .o .m

.m.o: ;	${CC} -c ${CPPFLAGS} -o ${.TARGET} ${.IMPSRC}

CPPFLAGS+=-I${.CURDIR}/../common -I${.CURDIR}/../arch \
	-I${.CURDIR}/../arch/${MACHINE_ARCH}


SRCS=	NXConstStr.m Object.m Protocol.m archive.c class.c \
	encoding.c hash.c init.c misc.c objects.c \
	sarray.c selector.c sendmsg.c 

.include <bsd.lib.mk>
