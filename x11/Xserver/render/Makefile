#	$NetBSD: Makefile,v 1.5 2004/02/07 06:52:08 lukem Exp $

.include "../Makefile.serverlib"

LIB=		render
.PATH:		${X11SRCDIR.xc}/programs/Xserver/render

SRCS=		animcur.c filter.c glyph.c miglyph.c miindex.c \
		mipict.c mirect.c mitrap.c mitri.c picture.c render.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/kdrive \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext

COPTS.miglyph.c=-Wno-error	# XXX gcc 3.3
COPTS.mirect.c=	-Wno-error	# XXX gcc 3.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
