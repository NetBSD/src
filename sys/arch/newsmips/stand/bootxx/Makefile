#	$NetBSD: Makefile,v 1.11.2.1 2009/05/13 17:18:10 jym Exp $

S!= cd ${.CURDIR}/../../../..; pwd

PROG= bootxx
SRCS= start.S bootxx.c
NOMAN= # defined
STRIPFLAG=
BINMODE= 444
WARNS?= 1
SIZE?= size

.include <bsd.own.mk>

COMMON!=  cd ${.CURDIR}/../common && ${PRINTOBJDIR}

LDSCRIPT= ${S}/arch/mips/conf/stand.ldscript
LINKFLAGS= -x -N -Ttext a0004000 -T ${LDSCRIPT} -e _start
CFLAGS= -Os -ffreestanding -mmemcpy -mno-abicalls -msoft-float -G 0
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

CPPFLAGS+= -D_STANDALONE #-DBOOTXX_DEBUG
CPPFLAGS+= -nostdinc -I. -I${S}

AFLAGS= -D_LOCORE -mno-abicalls

CLEANFILES+= machine mips ${PROG}.sym
CLEANFILES+= ${PROG}.bin

LIBS=  ${COMMON}/romcalls.o
LIBS+= ${COMMON}/lib/sa/libsa.a ${COMMON}/lib/kern/libkern.a

${PROG}: ${OBJS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${.TARGET}.sym ${OBJS} ${LIBS}
	${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}.bin
	mv ${.TARGET}.bin ${.TARGET}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
