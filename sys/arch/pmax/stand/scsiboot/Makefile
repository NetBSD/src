#	$NetBSD: Makefile,v 1.3 1999/01/22 14:05:06 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94


S=	${.CURDIR}/../../../..
PROG=	boot
SRCS=	start.S boot.c bootconf.c filesystem.c
CLEANFILES+=${PROG}.elf ${PROG}.map bootconf.c ${ALL}

DPADD+=	${LIBS}
NOMAN=	noman	# XXX: Should have one
LDBUG=	-T $S/arch/mips/conf/stand.ldscript

# Tell boot code which drivers we want to link against
SRCS+=	rz.c


ALL=	boot mkboot rzboot bootrz
#ALL+=	mkboottape	XXX not used now

all:	${ALL}

${PROG}: ${LIBS}

# before other deps on bootconf.o
bootconf.c: ${.CURDIR}/conf.c
	ln -s ${.CURDIR}/conf.c bootconf.c
	
bootconf.o: ${.CURDIR}/conf.c
	${CC} -c ${CFLAGS} ${CPPFLAGS} -DBOOT bootconf.c

# bootable from real disks
${PROG}: ${OBJS} ${LIBS}
	ld -Map ${PROG}.map -N -Ttext ${RELOC} ${LDBUG} -e start \
		${OBJS} ${LIBS} -o ${PROG}.elf
	elf2aout ${PROG}.elf ${PROG}

mkboot:	${.CURDIR}/mkboot.c
	${CC} ${CPPFLAGS} ${INCPATH} -o mkboot ${.CURDIR}/mkboot.c

mkboottape: ${.CURDIR}/mkboottape.c
	${CC} ${CPPFLAGS} ${INCPATH} -o mkboottape ${.CURDIR}/mkboottape.c

rzboot bootrz:	mkboot ${PROG}
	./mkboot ${PROG} rzboot bootrz

proginstall:: bootrz rzboot
	@echo "Sorry, bootblocks too large to install"
#	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
#		${.ALLSRC} ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
.include "../Makefile.inc" 	# XXX after <bsd.prog.mk> to reset CFLAGS, etc
.include "../Makefile.booters"
