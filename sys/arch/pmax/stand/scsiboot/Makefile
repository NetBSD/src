#	$NetBSD: Makefile,v 1.5 1999/02/22 10:53:13 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94


S=	${.CURDIR}/../../../..
PROG=	boot
SRCS=	start.S boot.c conf.c filesystem.c
CLEANFILES+=${PROG}.elf ${PROG}.map bootconf.c ${ALL}

DPADD+=	${LIBS}
MKMAN=	no
LDBUG=	-T $S/arch/mips/conf/stand.ldscript

# Tell boot code which drivers we want to link against
SRCS+=	rz.c


ALL=	boot mkboot rzboot bootrz

all:	${ALL}

${PROG}: ${OBJS} ${LIBS}
	ld -Map ${PROG}.map -N -Ttext ${RELOC} ${LDBUG} -e start \
		${OBJS} ${LIBS} -o ${PROG}.elf
	elf2aout ${PROG}.elf ${PROG}

mkboot:	${.CURDIR}/mkboot.c
	${CC} ${CPPFLAGS} ${INCPATH} -o mkboot ${.CURDIR}/mkboot.c

rzboot bootrz:	mkboot ${PROG}
	./mkboot ${PROG} rzboot bootrz

proginstall:: bootrz rzboot
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.ALLSRC} ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
.include "../Makefile.inc" 	# XXX after <bsd.prog.mk> to reset CFLAGS, etc
.include "../Makefile.booters"
