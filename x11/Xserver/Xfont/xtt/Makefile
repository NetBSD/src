#	$NetBSD: Makefile,v 1.7 2005/07/02 08:28:58 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"

LIB=	xtt

.PATH:	${X11SRCDIR.xc}/lib/font/FreeType/module
SRCS+=	xttmodule.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts/codeconv \
		-I${X11SRCDIR.xc}/lib/font/X-TrueType \
		-I${X11SRCDIR.xc}/extras/FreeType/lib \
		-I${X11SRCDIR.xc}/extras/FreeType/lib/extend \
		-I${X11SRCDIR.xc}/lib/font/FreeType \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/extras/X-TrueType \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/fontcache \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DTT_MAKE_OPTION_SINGLE_OBJECT=1 \
		-DTT_CONFIG_OPTION_EXTEND_ENGINE \
		-DOPT_ENCODINGS

.include <bsd.x11.mk>

.if !empty(X11FLAGS.LOADABLE)
CPPFLAGS+=	-DFONTMODULE
LIBDIR=	${XMODULEDIR}
.else
libinstall::
.endif

.include <bsd.lib.mk>
