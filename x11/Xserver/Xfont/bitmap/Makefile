#	$NetBSD: Makefile,v 1.3.4.2 2004/11/21 10:01:26 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

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

CPPFLAGS+=	-DBDFFORMAT -DSNFFORMAT -DPCFFORMAT \
		-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

CPPFLAGS.bitmapfunc.c=	-DX_GZIP_FONT_COMPRESSION

CPPFLAGS+=	-DFONTMODULE

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
