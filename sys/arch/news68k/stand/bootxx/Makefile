#	$NetBSD: Makefile,v 1.7.4.2 2002/06/23 17:38:47 jdolecek Exp $

NOMAN= # defined

.include <bsd.own.mk>

COMMON=	${.CURDIR}/../common
COMMONOBJ!=	cd ${COMMON} && ${PRINTOBJDIR}

.PATH:	${COMMON}

PROG= bootxx
SRCS= start.S bootxx.c
SRCS+= romcalls.S putchar.c
STRIPFLAG=
BINMODE= 444

SIZE?= size
OBJCOPY?= objcopy

S= ${.CURDIR}/../../../..

LDFLAGS= -x -N -Ttext 0 -e start
CFLAGS= -Os -Wall

CPPFLAGS+= -D_STANDALONE # -DBOOTXX_DEBUG
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S}
AFLAGS= -D_LOCORE

CLEANFILES+= machine m68k ${PROG}.sym

L= ${COMMONOBJ}/lib
LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a

.BEGIN: machine m68k
.NOPATH: machine m68k
realdepend realall: machine m68k

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

m68k::
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

${PROG}: ${OBJS} machine m68k
	${LD} ${LDFLAGS} -o ${.TARGET}.sym ${OBJS} ${LIBS}
	${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
