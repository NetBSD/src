#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:14 lukem Exp $

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		ps
.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xprint/ps

SRCS=		PsArc.c PsArea.c PsAttVal.c PsAttr.c PsCache.c PsColor.c \
		PsFonts.c PsGC.c PsInit.c PsLine.c PsMisc.c PsPixel.c \
		PsPixmap.c PsPolygon.c PsPrint.c PsSpans.c PsText.c \
		PsWindow.c psout.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/Xprint \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include

CPPFLAGS+=	-UXFree86LOADER -DXP_PSTEXT -D_XP_PRINT_SERVER_ -DBM_CACHE

.include <bsd.x11.mk>
.include <bsd.lib.mk>
