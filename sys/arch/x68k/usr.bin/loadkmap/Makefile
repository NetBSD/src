#	$NetBSD: Makefile,v 1.8.4.2 2002/01/08 00:28:51 nathanw Exp $
# Makefile for loadkmap

PROG=		loadkmap
NOMAN=		# defined
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
