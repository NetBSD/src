#	$NetBSD: Makefile.inc,v 1.4 2000/07/19 00:58:23 matt Exp $

XXRPB=0x1F4240
RELOC=0x2f0000
.PATH:	${.CURDIR}/../../vax ${.CURDIR}/../common

CPPFLAGS+=-I. -I${.CURDIR}/../../../../ -I${.CURDIR}/../../ -I${.CURDIR}/../common -I${.CURDIR}/../../include
CPPFLAGS+=-DXXRPB=${XXRPB} -DRELOC=${RELOC}

# Private handling of assembler files.
.s.o:
	${CC} -x assembler-with-cpp ${CPPFLAGS} -E ${.IMPSRC} | ${AS} -o ${.TARGET}
