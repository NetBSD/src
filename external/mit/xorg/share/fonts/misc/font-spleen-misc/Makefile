#	$NetBSD: Makefile,v 1.1 2020/10/10 07:28:04 nia Exp $

.include <bsd.own.mk>

FONTSUBDIR=	misc
FONTSRCPKG=	font-spleen-misc

X11SRCDIR.font-spleen-misc= \
	${X11SRCDIR}/external/bsd/${FONTSRCPKG}

BDFFILES+=	spleen-12x24.bdf
BDFFILES+=	spleen-16x32.bdf
BDFFILES+=	spleen-32x64.bdf
BDFFILES+=	spleen-5x8.bdf
BDFFILES+=	spleen-6x12.bdf
BDFFILES+=	spleen-8x16.bdf

.include "../../Makefile.bdf"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
