#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

LIB=		raster
.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xprint/raster

SRCS=		Raster.c RasterAttVal.c

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xprint \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include \
		-I${X11SRCDIR.xc}/include/fonts

CPPFLAGS+=	-D_XP_PRINT_SERVER_ -UXFree86LOADER

.include <bsd.lib.mk>
.include <bsd.x11.mk>
