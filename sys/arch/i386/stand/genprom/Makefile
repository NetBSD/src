#	$NetBSD: Makefile,v 1.1.1.1.4.1 1997/08/23 07:09:28 thorpej Exp $

PROG= genprom
NOMAN=

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	dd if=${BINFILE} ibs=32 skip=1 | ./${PROG} ${ROM_SIZE} >${ROMFILE} \
		|| (rm ${ROMFILE}; false)
