#	$NetBSD: Makefile,v 1.1 1999/07/08 11:48:06 tsubai Exp $

PROG= bootxx
SRCS= start.S romcalls.S bootxx.c
MKMAN= no
STRIPFLAG=
BINMODE= 444
SIZE?= size

LDFLAGS= -x -N -Ttext a0000000
CFLAGS= -Os -mno-abicalls -G 0 -Wall

CPPFLAGS+= -D_STANDALONE #-DBOOTXX_DEBUG
CPPFLAGS+= -I../../../..
AFLAGS= -D_LOCORE

L= ../boot/lib
LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a

${PROG}: ${OBJS}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}
	${SIZE} ${PROG}

.include <bsd.prog.mk>
