#	$NetBSD: Makefile,v 1.4.16.1 2002/01/08 00:25:47 nathanw Exp $

PROG= genprom
NOMAN=	# defined

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	./${PROG} ${ROM_SIZE} <${BINFILE} >${ROMFILE} || (rm ${ROMFILE}; false)
