#	$NetBSD: Makefile,v 1.4.18.1 2002/01/10 19:45:12 thorpej Exp $

PROG= genprom
NOMAN=	# defined

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	./${PROG} ${ROM_SIZE} <${BINFILE} >${ROMFILE} || (rm ${ROMFILE}; false)
