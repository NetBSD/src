#	$NetBSD: Makefile,v 1.2 1999/01/21 14:16:52 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94


S=	${.CURDIR}/../../../..
PROG=	boot
NOMAN=	noman	# XXX: Should have one
LDBUG=	-T $S/arch/mips/conf/stand.ldscript


ALL=	boot mkboot rzboot bootrz
#ALL+=	mkboottape	XXX not used now

all:	${ALL}

boot:	${LIBS}
DPADD+=	${LIBS}

.PATH:	${.CURDIR}/../libsa

# Tell boot code which drivers we want to link against
DRIVERS=rz.c
LIBS+=	libdrive.a


#before other deps on bootconf.o
bootconf.o: ${.CURDIR}/conf.c
	rm -f bootconf.c
	ln -s ${.CURDIR}/conf.c bootconf.c
	${CC} -c ${CFLAGS} -DBOOT bootconf.c
	rm -f bootconf.c


# bootable from real disks

boot:	start.o boot.o bootconf.o filesystem.o ${LIBS}
	ld -Map boot.map -N -Ttext ${RELOC} ${LDBUG} -e start \
		start.o boot.o bootconf.o filesystem.o ${LIBS} -o boot.elf
	elf2aout boot.elf boot

start.o: ${.CURDIR}/start.S

mkboot:	${.CURDIR}/mkboot.c
	${CC} ${CPPFLAGS} ${INCPATH} -o mkboot ${.CURDIR}/mkboot.c

mkboottape: ${.CURDIR}/mkboottape.c
	${CC} ${CPPFLAGS} ${INCPATH} -o mkboottape ${.CURDIR}/mkboottape.c

rzboot bootrz:	mkboot boot
	./mkboot boot rzboot bootrz

# utilities

clean cleandir:
	rm -f .depend *.o *.exe *.i errs make.out core*
	rm -f a.out boot.elf ${ALL}
	rm -f boot[a-z][a-z] [a-z][a-z]boot
	rm -f libdrive.a
	rm -f boot.map

proginstall:: bootrz rzboot
	@echo "Sorry, bootblocks too large to install"
#	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
#		${.ALLSRC} ${DESTDIR}${BINDIR}

.include "../Makefile.booters"
.include <bsd.prog.mk>
.include "../Makefile.inc" 	# XXX after to reset CFLAGS, etc
