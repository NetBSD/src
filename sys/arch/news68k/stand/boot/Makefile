#	$NetBSD: Makefile,v 1.27 2009/03/20 13:59:49 tsutsui Exp $

NOMAN= # defined

.include <bsd.own.mk>

COMMON= ${.CURDIR}/../common
COMMONOBJ!=	cd ${COMMON} && ${PRINTOBJDIR}

.PATH: ${COMMON}

PROG= boot
SRCS= locore.S boot.c devopen.c
STRIPFLAG=
BINMODE= 444

S= ${.CURDIR}/../../../..

LINKFLAGS= -x -N -Ttext 3e0000 -e start
CFLAGS= -Os -Wall -Werror
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
#CFLAGS+= -Wno-format-zero-length -Wno-sign-compare

CPPFLAGS+= -D_STANDALONE # -DBOOT_DEBUG
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S}
CPPFLAGS+= -D__daddr_t=int32_t
AFLAGS= -D_LOCORE

CLEANFILES+= ${PROG}.sym

L= ${COMMONOBJ}/lib
LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a

${PROG}: ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${.TARGET}.sym ${OBJS} ${LIBS}
	-${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
