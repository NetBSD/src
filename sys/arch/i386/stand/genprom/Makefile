#	$NetBSD: Makefile,v 1.2 1997/08/20 16:53:30 drochner Exp $

PROG= genprom
NOMAN=

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	dd if=${BINFILE} ibs=32 skip=1 | ./${PROG} ${ROM_SIZE} >${ROMFILE} \
		|| (rm ${ROMFILE}; false)
