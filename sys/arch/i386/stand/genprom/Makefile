#	$NetBSD: Makefile,v 1.4 1999/07/08 14:32:29 drochner Exp $

PROG= genprom
MKMAN=	no

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	./${PROG} ${ROM_SIZE} <${BINFILE} >${ROMFILE} || (rm ${ROMFILE}; false)
