#	$NetBSD: Makefile,v 1.8.8.2 2002/06/23 17:38:52 jdolecek Exp $

S= ${.CURDIR}/../../../..

PROG= boot
SRCS= locore.S boot.c bootinfo.c devopen.c net.c netif_news.c
NOMAN= # defined
STRIPFLAG=
BINMODE= 444

SIZE?= size
STRIP?= strip

.include <bsd.own.mk>

COMMON!=  cd ${.CURDIR}/../common && ${PRINTOBJDIR}

CLEANFILES+= ${PROG}.elf ${PROG}.tmp

LDFLAGS= -x -N -Ttext a0700000 -e _start
CFLAGS= -Os -mmemcpy -mno-abicalls -G 0 -Wall

CPPFLAGS+= -DSUPPORT_USTARFS -DHAVE_CHANGEDISK_HOOK
CPPFLAGS+= -DSUN_BOOTPARAMS
CPPFLAGS+= -D_STANDALONE #-DBOOT_DEBUG
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
