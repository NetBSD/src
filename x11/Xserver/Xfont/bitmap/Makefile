#	$NetBSD: Makefile,v 1.2 2004/02/07 06:52:04 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"

LIB=	bitmap
.PATH:	${X11SRCDIR.xc}/lib/font/bitmap
.PATH:	${X11SRCDIR.xc}/lib/font/bitmap/module

SRCS=	bdfread.c bdfutils.c bitmap.c bitmaputil.c bitscale.c bitmapfunc.c \
	pcfread.c pcfwrite.c snfread.c fontink.c bitmapmod.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DBDFFORMAT -DSNFFORMAT -DPCFFORMAT -DFONTMODULE

CPPFLAGS.bitmapfunc.c=	-DX_GZIP_FONT_COMPRESSION

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
