#	$NetBSD: Makefile,v 1.4.2.3 2002/04/17 00:04:04 nathanw Exp $

PROG= bootxx
SRCS= start.S bootxx.c
NOMAN= # defined
STRIPFLAG=
BINMODE= 444
SIZE?= size

.include <bsd.own.mk>

COMMON!=  cd ${.CURDIR}/../common && ${PRINTOBJDIR}

LDFLAGS= -x -N -Ttext a0004000 -e _start
CFLAGS= -Os -mmemcpy -mno-abicalls -G 0 -Wall

CPPFLAGS+= -D_STANDALONE #-DBOOTXX_DEBUG
CPPFLAGS+= -I${COMMON} -I${.CURDIR}/../../../..

AFLAGS= -D_LOCORE

LIBS=  ${COMMON}/romcalls.o
LIBS+= ${COMMON}/lib/sa/libsa.a ${COMMON}/lib/kern/libkern.a

${PROG}: ${OBJS}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}
	${SIZE} ${PROG}

.include <bsd.prog.mk>
