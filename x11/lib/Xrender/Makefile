#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:50 lukem Exp $

.include <bsd.own.mk>

LIB=	Xrender
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Color.c Composite.c Cursor.c FillRect.c FillRects.c Filter.c \
	Glyph.c Picture.c Poly.c Trap.c Tri.c Xrender.c

INCS=	Xrender.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11/extensions
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/X11

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
