#	$NetBSD: Makefile,v 1.5 2001/12/12 01:49:44 tv Exp $

PROG= genprom
NOMAN=	# defined

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	./${PROG} ${ROM_SIZE} <${BINFILE} >${ROMFILE} || (rm ${ROMFILE}; false)
