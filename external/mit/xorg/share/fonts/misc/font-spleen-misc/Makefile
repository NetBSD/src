#	$NetBSD: Makefile,v 1.2 2020/10/11 17:49:08 nia Exp $

.include <bsd.own.mk>

FONTSUBDIR=	misc
FONTSRCPKG=	font-spleen-misc

X11SRCDIR.font-spleen-misc= \
	${X11SRCDIR}/external/bsd/${FONTSRCPKG}

.for size in 32x64 16x32 12x24 8x16 6x12 5x8
BDFFILES+=	spleen-${size}.bdf
UCSFILES+=	spleen-${size} ISO8859-1
.endfor

.include "../../Makefile.bdf"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
