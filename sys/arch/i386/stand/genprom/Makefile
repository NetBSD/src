#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:46 lukem Exp $

PROG= genprom
MKMAN=	no

CFLAGS+= -Wall

.include <bsd.prog.mk>

#to be used from netboot's Makefile
rom: ${PROG} ${BINFILE}
	dd if=${BINFILE} ibs=32 skip=1 | ./${PROG} ${ROM_SIZE} >${ROMFILE} \
		|| (rm ${ROMFILE}; false)
