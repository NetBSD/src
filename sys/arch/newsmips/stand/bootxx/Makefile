#	$NetBSD: Makefile,v 1.4.2.4 2002/06/20 03:40:19 nathanw Exp $

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
CPPFLAGS+= -nostdinc -I${COMMON} -I${.CURDIR}/../../../..

AFLAGS= -D_LOCORE

CLEANFILES+= machine mips ${PROG}.sym
CLEANFILES+= ${PROG}.bin ${PROG}.tmp

LIBS=  ${COMMON}/romcalls.o
LIBS+= ${COMMON}/lib/sa/libsa.a ${COMMON}/lib/kern/libkern.a

${PROG}: ${OBJS}
	${LD} ${LDFLAGS} -o ${.TARGET}.sym ${OBJS} ${LIBS}
	${SIZE} ${.TARGET}.sym
# XXX objcopy does not work for prog at KSEG
#	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}
	${STRIP} ${.TARGET}.sym -o ${.TARGET}.tmp
	tail -c +177 ${.TARGET}.tmp > ${.TARGET}.bin
	mv ${.TARGET}.bin ${.TARGET}

.include <bsd.prog.mk>
