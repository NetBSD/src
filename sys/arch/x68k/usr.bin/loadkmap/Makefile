#	$NetBSD: Makefile,v 1.2 1997/06/22 07:09:04 mrg Exp $
# Makefile for loadkmap

all: loadkmap ascii_kmap jis_kmap

ascii_kmap: ascii_kmap.c
	$(CC) -c -o ${.TARGET}.aout ${.CURDIR}/ascii_kmap.c
	strip ${.TARGET}.aout
	dd bs=1 skip=32 if=${.TARGET}.aout of=${.TARGET}
	rm ${.TARGET}.aout

jis_kmap: jis_kmap.c
	$(CC) -c -o ${.TARGET}.aout ${.CURDIR}/jis_kmap.c
	strip ${.TARGET}.aout
	dd bs=1 skip=32 if=${.TARGET}.aout of=${.TARGET}
	rm ${.TARGET}.aout

NOMAN=	yes
PROG = loadkmap

.include <bsd.prog.mk>
