#	$NetBSD: Makefile,v 1.6 2004/08/05 01:28:48 lukem Exp $

.include <bsd.own.mk>

LIB=	Xrender
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Color.c Composite.c Cursor.c FillRect.c FillRects.c Filter.c \
	Glyph.c Picture.c Poly.c Trap.c Tri.c Xrender.c

INCS=	Xrender.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/X11

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
