#	$NetBSD: Makefile.inc,v 1.3 1999/03/03 12:00:18 jonathan Exp $
#
# This Makefile includes boiler-plate stuff included by each subdir's Makefile.

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

LIB=    ${MACHINE_ARCH:S/mipse[bl]/mips/}
