#	$NetBSD: Makefile,v 1.5 1999/12/23 01:40:06 tsubai Exp $

S= ${.CURDIR}/../../../..

PROG= boot
SRCS= locore.S boot.c bootinfo.c devopen.c net.c netif_news.c
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

CPPFLAGS+= -DSUN_BOOTPARAMS
CPPFLAGS+= -D_STANDALONE -DBOOT_DEBUG
CPPFLAGS+= -I${.CURDIR} -I${COMMON} -I${S}

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
