#	$NetBSD: Makefile,v 1.1 2024/09/22 09:34:57 mrg Exp $

.include <bsd.own.mk>

FONTSUBDIR=	misc
FONTSRCPKG=	font-sun-misc

.PATH:  	${X11SRCDIR.${FONTSRCPKG}}
BDFFILES+=	olcursor.bdf \
		olgl10.bdf \
		olgl12.bdf \
		olgl14.bdf \
		olgl19.bdf

.include "../../Makefile.bdf"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
