#	$NetBSD: Makefile,v 1.10 2003/07/16 07:25:51 he Exp $
# Makefile for loadkmap

PROG=		loadkmap
NOMAN=		# defined
FILES=		ascii_kmap jis_kmap
FILESDIR=	/usr/share/keymaps/x68k

CLEANFILES+=	ascii_kmap jis_kmap

realall:	loadkmap ascii_kmap jis_kmap

ascii_kmap: ascii_kmap.c
	${COMPILE.c} -o ${.TARGET}.aout ${.CURDIR}/ascii_kmap.c
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}
	rm ${.TARGET}.aout

jis_kmap: jis_kmap.c
	${COMPILE.c} -o ${.TARGET}.aout ${.CURDIR}/jis_kmap.c
	${OBJCOPY} -O binary ${.TARGET}.aout ${.TARGET}
	rm ${.TARGET}.aout

.include <bsd.prog.mk>
