#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:40 lukem Exp $

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

#LM#	LDADD+=	-lXext -lX11
#LM#	DPADD+=	${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
