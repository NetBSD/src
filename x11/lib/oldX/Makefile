#	$NetBSD: Makefile,v 1.5 2004/08/04 08:37:05 lukem Exp $

.include <bsd.own.mk>

LIB=	oldX
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XCrAssoc.c XDelAssoc.c XDestAssoc.c XDraw.c XLookAssoc.c XMakeAssoc.c

INCS=	X10.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LIBDPLIBS=\
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
