#	$NetBSD: Makefile,v 1.2 2024/09/22 13:58:22 jakllsch Exp $

.include <bsd.own.mk>

FONTSUBDIR=	misc
FONTSRCPKG=	font-schumacher-misc

.PATH:  	${X11SRCDIR.${FONTSRCPKG}}
BDFFILES+=	clB6x10.bdf \
		clB6x12.bdf \
		clB8x10.bdf \
		clB8x12.bdf \
		clB8x13.bdf \
		clB8x14.bdf \
		clB8x16.bdf \
		clB8x8.bdf \
		clB9x15.bdf \
		clI6x12.bdf \
		clI8x8.bdf \
		clR4x6.bdf \
		clR5x10.bdf \
		clR5x6.bdf \
		clR5x8.bdf \
		clR6x10.bdf \
		clR6x12.bdf \
		clR6x13.bdf \
		clR6x6.bdf \
		clR6x8.bdf \
		clR7x10.bdf \
		clR7x12.bdf \
		clR7x14.bdf \
		clR7x8.bdf \
		clR8x10.bdf \
		clR8x12.bdf \
		clR8x13.bdf \
		clR8x14.bdf \
		clR8x16.bdf \
		clR8x8.bdf \
		clR9x15.bdf

ISOS=		ISO8859-1 ISO8859-2 ISO8859-3 ISO8859-4 ISO8859-5 \
		ISO8859-7 ISO8859-8 ISO8859-9 ISO8859-10 ISO8859-13 \
		ISO8859-14 ISO8859-15 ISO8859-16 KOI8-R
.for _l in	${ISOS}
UCSFILES+=	clR6x12 ${_l}
.endfor

.include "../../Makefile.bdf"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
