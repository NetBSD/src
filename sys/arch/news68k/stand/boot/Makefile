#	$NetBSD: Makefile,v 1.22 2005/12/29 06:49:11 skrll Exp $

NOMAN= # defined

.include <bsd.own.mk>

COMMON= ${.CURDIR}/../common
COMMONOBJ!=	cd ${COMMON} && ${PRINTOBJDIR}

.PATH: ${COMMON}

PROG= boot
SRCS= locore.S boot.c devopen.c
SRCS+= romcalls.S getchar.c putchar.c
STRIPFLAG=
BINMODE= 444

S= ${.CURDIR}/../../../..

LDFLAGS= -x -N -Ttext 3e0000 -e start
CFLAGS= -Os -Wall

CPPFLAGS+= -D_STANDALONE # -DBOOT_DEBUG
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S}
CPPFLAGS+= -D__daddr_t=int32_t
AFLAGS= -D_LOCORE

CLEANFILES+= ${PROG}.sym machine m68k

L= ${COMMONOBJ}/lib
LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a

.if !make(obj) && !make(clean) && !make(cleandir)
.NOPATH: machine m68k
.BEGIN: machine m68k

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

m68k::
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@
.endif

${PROG}: ${OBJS} ${LIBS}
	${LD} ${LDFLAGS} -o ${.TARGET}.sym ${OBJS} ${LIBS}
	-${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
