#	$NetBSD: Makefile,v 1.1.2.1 1999/12/27 18:33:03 wrstuden Exp $

COMMON= ${.CURDIR}/../common
.ifnmake(print-objdir)
COMMONOBJ!=	cd ${COMMON}; ${MAKE} print-objdir
.endif

.PATH: ${COMMON}

PROG= boot
SRCS= locore.S boot.c devopen.c
SRCS+= romcalls.S putchar.c
MKMAN= no
STRIPFLAG=
BINMODE= 444

SIZE?= size
STRIP?= strip

S= ${.CURDIR}/../../../..

LDFLAGS= -x -N -Ttext 3e0000
CFLAGS= -Os -Wall

CPPFLAGS+= -D_STANDALONE -DBOOT_DEBUG
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S}
AFLAGS= -D_LOCORE

CLEANFILES+= ${PROG}.tmp ${.OBJDIR}/machine ${.OBJDIR}/m68k

L= ${COMMONOBJ}/lib
LIBS= ${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a

.BEGIN: ${.OBJDIR}/machine ${.OBJDIR}/m68k

depend all: ${.OBJDIR}/machine ${.OBJDIR}/m68k

${.OBJDIR}/machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

${.OBJDIR}/m68k::
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

${PROG}: ${OBJS} ${.OBJDIR}/machine ${.OBJDIR}/m68k
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBS}
	-${SIZE} ${PROG}
	${STRIP} ${PROG}
	tail -c +33 ${PROG} > ${PROG}.tmp
	mv ${PROG}.tmp ${PROG}

.include <bsd.prog.mk>
