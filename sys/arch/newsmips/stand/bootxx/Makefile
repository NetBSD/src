#	$NetBSD: Makefile,v 1.2 1999/12/17 07:40:10 tsubai Exp $

PROG= bootxx
SRCS= start.S bootxx.c
MKMAN= no
STRIPFLAG=
BINMODE= 444
SIZE?= size

COMMON!=  cd ${.CURDIR}/../common && ${MAKE} print-objdir

LDFLAGS= -x -N -Ttext a0000000 -e _start
CFLAGS= -Os -mno-abicalls -G 0 -Wall

CPPFLAGS+= -D_STANDALONE #-DBOOTXX_DEBUG
CPPFLAGS+= -I${COMMON} -I${.CURDIR}/../../../..

AFLAGS= -D_LOCORE

LIBS=  ${COMMON}/romcalls.o
LIBS+= ${COMMON}/lib/sa/libsa.a ${COMMON}/lib/kern/libkern.a

${PROG}: ${OBJS}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}
	${SIZE} ${PROG}

.include <bsd.prog.mk>
