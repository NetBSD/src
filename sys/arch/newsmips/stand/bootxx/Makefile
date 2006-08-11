#	$NetBSD: Makefile,v 1.8.40.1 2006/08/11 15:42:23 yamt Exp $

PROG= bootxx
SRCS= start.S bootxx.c
NOMAN= # defined
STRIPFLAG=
BINMODE= 444
WARNS?= 1
SIZE?= size

.include <bsd.own.mk>

COMMON!=  cd ${.CURDIR}/../common && ${PRINTOBJDIR}

LINKFLAGS= -x -N -Ttext a0004000 -e _start
CFLAGS= -Os -ffreestanding -mmemcpy -mno-abicalls -msoft-float -G 0

CPPFLAGS+= -D_STANDALONE #-DBOOTXX_DEBUG
CPPFLAGS+= -nostdinc -I${COMMON} -I${.CURDIR}/../../../..

AFLAGS= -D_LOCORE -mno-abicalls

CLEANFILES+= machine mips ${PROG}.sym
CLEANFILES+= ${PROG}.bin ${PROG}.tmp

LIBS=  ${COMMON}/romcalls.o
LIBS+= ${COMMON}/lib/sa/libsa.a ${COMMON}/lib/kern/libkern.a

${PROG}: ${OBJS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${.TARGET}.sym ${OBJS} ${LIBS}
	${SIZE} ${.TARGET}.sym
# XXX objcopy does not work for prog at KSEG
#	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}
	${STRIP} ${.TARGET}.sym -o ${.TARGET}.tmp
	tail -c +177 ${.TARGET}.tmp > ${.TARGET}.bin
	mv ${.TARGET}.bin ${.TARGET}

.include <bsd.prog.mk>
