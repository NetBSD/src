#	$NetBSD: Makefile.inc,v 1.4 1999/03/05 12:19:17 drochner Exp $
#
# This Makefile includes boiler-plate stuff included by each subdir's Makefile.

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

LIB=    ${MACHINE_ARCH:C/mipse[bl]/mips/}
