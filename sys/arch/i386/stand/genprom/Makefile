#	$NetBSD: Makefile,v 1.3.4.1 1999/08/02 19:50:36 thorpej Exp $

PROG= genprom
MKMAN=	no

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	./${PROG} ${ROM_SIZE} <${BINFILE} >${ROMFILE} || (rm ${ROMFILE}; false)
