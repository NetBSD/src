#	$NetBSD: Makefile,v 1.3.4.1 2004/11/21 10:01:28 rtr Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		raster

.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xprint/raster
SRCS=		Raster.c RasterAttVal.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xprint \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include \
		-I${X11SRCDIR.xc}/include/fonts

CPPFLAGS+=	-D_XP_PRINT_SERVER_ -UXFree86LOADER

.include <bsd.x11.mk>
.include <bsd.lib.mk>
