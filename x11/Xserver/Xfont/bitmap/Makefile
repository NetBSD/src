#	$NetBSD: Makefile,v 1.3 2004/03/16 17:40:53 matt Exp $

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

CPPFLAGS+=	-DBDFFORMAT -DSNFFORMAT -DPCFFORMAT

CPPFLAGS.bitmapfunc.c=	-DX_GZIP_FONT_COMPRESSION

.include <bsd.x11.mk>

.if !empty(X11FLAGS.LOADABLE)
CPPFLAGS+=	-DFONTMODULE
LIBDIR=	${XMODULEDIR}
.else
libinstall::
.endif

.include <bsd.lib.mk>
