#	$NetBSD: Makefile,v 1.2 1999/12/17 07:40:10 tsubai Exp $

S= ${.CURDIR}/../../../..

PROG= boot
SRCS= locore.S boot.c devopen.c
MKMAN= no
STRIPFLAG=
BINMODE= 444

OBJCOPY?= objcopy
SIZE?= size
STRIP?= strip

COMMON!=  cd ${.CURDIR}/../common && ${MAKE} print-objdir

CLEANFILES+= ${PROG}.elf ${PROG}.tmp

LDFLAGS= -x -N -Ttext a0700000 -e _start
CFLAGS= -Os -G 0 -mno-abicalls -Wall

CPPFLAGS+= -D_STANDALONE #-DBOOT_DEBUG
CPPFLAGS+= -I${COMMON} -I${S}

AFLAGS= -D_LOCORE

LIBS = ${COMMON}/romcalls.o
LIBS+= ${COMMON}/lib/sa/libsa.a
LIBS+= ${COMMON}/lib/z/libz.a
LIBS+= ${COMMON}/lib/kern/libkern.a

${PROG}: ${OBJS}
	${LD} ${LDFLAGS} -o ${PROG}.elf ${OBJS} ${LIBS}
	-${SIZE} ${PROG}.elf
	${STRIP} ${PROG}.elf -o ${PROG}
	tail -c +177 ${PROG} > ${PROG}.tmp
	mv ${PROG}.tmp ${PROG}
#	${OBJCOPY} -O binary ${PROG}.elf ${PROG}	# XXX

.include <bsd.prog.mk>
