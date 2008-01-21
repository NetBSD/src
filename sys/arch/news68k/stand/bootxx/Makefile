#	$NetBSD: Makefile,v 1.13.2.3 2008/01/21 09:37:57 yamt Exp $

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

S!= cd ${.CURDIR}/../../../..; pwd

LINKFLAGS= -x -N -Ttext 0 -e start
CFLAGS= -Os -Wall -Werror
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
#CFLAGS+= -Wno-format-zero-length -Wno-sign-compare

CPPFLAGS+= -D_STANDALONE # -DBOOTXX_DEBUG
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S}
AFLAGS= -D_LOCORE

CLEANFILES+= machine m68k ${PROG}.sym

L= ${COMMONOBJ}/lib
LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN: machine m68k
.NOPATH: machine m68k

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

m68k::
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@
.endif

${PROG}: ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${.TARGET}.sym ${OBJS} ${LIBS}
	${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
