#	$NetBSD: Makefile.inc,v 1.3 2000/07/03 04:24:12 matt Exp $

XXRPB=0x1F4240
RELOC=0x2f0000
.PATH:	${.CURDIR}/../../vax ${.CURDIR}/../common

CPPFLAGS+=-I${.CURDIR}/../../../../ -I${.CURDIR}/../../ -I${.CURDIR}/../common -I${.CURDIR}/../../include
CPPFLAGS+=-DXXRPB=${XXRPB} -DRELOC=${RELOC}

# Private handling of assembler files.
.s.o:
	${CC} -x assembler-with-cpp ${CPPFLAGS} -E ${.IMPSRC} | ${AS} -o ${.TARGET}
