#	$NetBSD: Makefile,v 1.5.8.2 2000/12/08 09:30:56 bouyer Exp $
# Makefile for loadkmap

PROG=		loadkmap
MKMAN=		no
FILES=		ascii_kmap jis_kmap
FILESDIR=	/usr/share/keymaps/x68k

CLEANFILES+=	ascii_kmap jis_kmap

realall:	loadkmap ascii_kmap jis_kmap

OBJCOPY?=	/usr/bin/objcopy

ascii_kmap: ascii_kmap.c
	$(CC) -c -o ${.TARGET}.aout ${.CURDIR}/ascii_kmap.c
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}
	rm ${.TARGET}.aout

jis_kmap: jis_kmap.c
	$(CC) -c -o ${.TARGET}.aout ${.CURDIR}/jis_kmap.c
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}
	rm ${.TARGET}.aout

.include <bsd.prog.mk>
