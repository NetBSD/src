#	$NetBSD: Makefile,v 1.8 2001/09/22 03:34:21 tv Exp $
# Makefile for loadkmap

PROG=		loadkmap
MKMAN=		no
FILES=		ascii_kmap jis_kmap
FILESDIR=	/usr/share/keymaps/x68k

CLEANFILES+=	ascii_kmap jis_kmap

realall:	loadkmap ascii_kmap jis_kmap

ascii_kmap: ascii_kmap.c
	$(CC) -c -o ${.TARGET}.aout ${.CURDIR}/ascii_kmap.c
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}
	rm ${.TARGET}.aout

jis_kmap: jis_kmap.c
	$(CC) -c -o ${.TARGET}.aout ${.CURDIR}/jis_kmap.c
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}
	rm ${.TARGET}.aout

.include <bsd.prog.mk>
