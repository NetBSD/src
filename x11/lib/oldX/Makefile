#	$NetBSD: Makefile,v 1.6 2004/08/05 01:28:48 lukem Exp $

.include <bsd.own.mk>

LIB=	oldX
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XCrAssoc.c XDelAssoc.c XDestAssoc.c XDraw.c XLookAssoc.c XMakeAssoc.c

INCS=	X10.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LIBDPLIBS=\
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
