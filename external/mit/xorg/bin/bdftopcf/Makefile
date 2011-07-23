#	$NetBSD: Makefile,v 1.2 2011/07/23 23:42:02 mrg Exp $

.include <bsd.own.mk>

PROG=	bdftopcf

LDADD+=	-lXfont -lfontenc -lfreetype -lm -lz -lbz2
DPADD+=	${LIBXFONT} ${LIBFONTENC} ${LIBFREETYPE} ${LIBM} ${LIBZ} ${LIBBZ2}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
