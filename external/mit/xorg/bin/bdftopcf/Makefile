#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	bdftopcf

LDADD+=	-lXfont -lfontenc -lfreetype -lm -lz
DPADD+=	${LIBXFONT} ${LIBFONTENC} ${LIBFREETYPE} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
