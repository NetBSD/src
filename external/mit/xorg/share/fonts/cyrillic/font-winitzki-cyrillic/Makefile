#	$NetBSD: Makefile,v 1.1 2024/09/22 09:34:56 mrg Exp $

.include <bsd.own.mk>

FONTSUBDIR=	cyrillic
FONTSRCPKG=	font-winitzki-cyrillic

.PATH:  	${X11SRCDIR.${FONTSRCPKG}}
BDFFILES+=	proof9x16.bdf

.include "../../Makefile.bdf"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
