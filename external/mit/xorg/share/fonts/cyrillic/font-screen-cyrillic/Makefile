#	$NetBSD: Makefile,v 1.1 2024/09/22 09:34:56 mrg Exp $

.include <bsd.own.mk>

FONTSUBDIR=	cyrillic
FONTSRCPKG=	font-screen-cyrillic

.PATH:  	${X11SRCDIR.${FONTSRCPKG}}
BDFFILES+=	screen8x16.bdf \
		screen8x16b.bdf

.include "../../Makefile.bdf"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
