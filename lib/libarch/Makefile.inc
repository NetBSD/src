#	$NetBSD: Makefile.inc,v 1.2 1999/02/09 22:32:33 is Exp $
#
# This Makefile includes boiler-plate stuff included by each subdir's Makefile.

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

LIB=    ${MACHINE_ARCH}
