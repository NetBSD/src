#	$NetBSD: Makefile.inc,v 1.1 1999/03/06 16:36:04 ragge Exp $

XXRPB=0x1F4240
RELOC=0x2f0000
.PATH:	../../vax ../common

CPPFLAGS+=-I../../../../ -I../../ -I../common -I../../include
CPPFLAGS+=-DXXRPB=${XXRPB} -DRELOC=${RELOC}

# Private handling of assembler files.
.s.o:
	${CC} -x assembler-with-cpp ${CPPFLAGS} -E ${.IMPSRC} | as -o ${.TARGET}
