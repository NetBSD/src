#	$NetBSD: Makefile,v 1.4 1999/02/13 02:54:50 lukem Exp $
# Makefile for loadkmap

PROG=		loadkmap
MKMAN=		no
FILES=		ascii_kmap jis_kmap
FILESDIR=	/usr/share/keymaps/x68k

CLEANFILES+=	ascii_kmap jis_kmap

all:		loadkmap ascii_kmap jis_kmap

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

.include <bsd.prog.mk>
