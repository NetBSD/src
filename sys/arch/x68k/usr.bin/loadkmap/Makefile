#	$NetBSD: Makefile,v 1.1.1.1 1996/05/05 12:17:03 oki Exp $
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

PROG = loadkmap

.include <bsd.prog.mk>
