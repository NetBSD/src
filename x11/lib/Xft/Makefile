#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:47 lukem Exp $

#XTODO	xft.pc
#XTODO	xft-config

.include <bsd.own.mk>

LIB=	Xft
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	xftcolor.c xftcore.c xftdbg.c xftdpy.c xftdraw.c xftextent.c \
	xftfont.c xftfreetype.c xftglyphs.c xftinit.c xftlist.c \
	xftname.c xftrender.c xftstr.c xftswap.c xftxlfd.c

INCS=	Xft.h XftCompat.h
INCSDIR=${X11INCDIR}/X11/Xft

MAN=	Xft.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}/${X11INCDIR}/freetype2

.include <bsd.x11.mk>
.include <bsd.lib.mk>
