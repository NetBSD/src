#	$NetBSD: Makefile,v 1.3 1998/08/04 16:39:55 minoura Exp $
# Makefile for loadkmap

PROG=		loadkmap
NOMAN=		yes
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
