#	$NetBSD: Makefile,v 1.2 2004/02/07 06:52:07 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../Makefile.serverlib"

LIB=	layer
.PATH:	${X11SRCDIR.xc}/programs/Xserver/miext/${LIB}

SRCS=	layergc.c layerinit.c layerpict.c layerwin.c

.if defined(X11FLAGS.LOADABLE)
SRCS+=	laymodule.c
.endif

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/miext/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/shadow \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
