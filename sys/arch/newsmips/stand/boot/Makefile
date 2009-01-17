#	$NetBSD: Makefile,v 1.18.58.1 2009/01/17 13:28:22 mjf Exp $

S= ${.CURDIR}/../../../..

PROG= boot
SRCS= locore.S boot.c bootinfo.c devopen.c net.c netif_news.c
NOMAN= # defined
STRIPFLAG=
BINMODE= 444
WARNS?= 1

.include <bsd.own.mk>

COMMON!=  cd ${.CURDIR}/../common && ${PRINTOBJDIR}

CLEANFILES+= ${PROG}.elf ${PROG}.tmp

LINKFLAGS= -x -N -Ttext a0700000 -e _start
LINKFLAGS+= -T ${S}/arch/mips/conf/stand.ldscript
CFLAGS= -Os -ffreestanding -mmemcpy -mno-abicalls -msoft-float -G 0
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

CPPFLAGS+= -DSUPPORT_USTARFS -DHAVE_CHANGEDISK_HOOK
CPPFLAGS+= -DSUN_BOOTPARAMS
CPPFLAGS+= -D_STANDALONE #-DBOOT_DEBUG
CPPFLAGS+= -I${.CURDIR} -I${COMMON} -I${S}
CPPFLAGS+= -D__daddr_t=int32_t

AFLAGS= -D_LOCORE -mno-abicalls

LIBS = ${COMMON}/romcalls.o
LIBS+= ${COMMON}/lib/sa/libsa.a
LIBS+= ${COMMON}/lib/z/libz.a
LIBS+= ${COMMON}/lib/kern/libkern.a

${PROG}: ${OBJS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${PROG}.elf ${OBJS} ${LIBS}
	-${SIZE} ${PROG}.elf
	${OBJCOPY} -O binary ${PROG}.elf ${PROG}

.include <bsd.prog.mk>
