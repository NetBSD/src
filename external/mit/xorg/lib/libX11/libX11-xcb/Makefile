#	$NetBSD: Makefile,v 1.1 2011/02/23 07:53:30 mrg Exp $

NOLINT=		1

.include <bsd.own.mk>

LIB=	X11-xcb

SRCS+=	x11_xcb.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LIBDPLIBS+= \
	X11 ${.CURDIR}/../../libX11/dynamic

.PATH:	${X11SRCDIR.X11}/src

.include <bsd.x11.mk>
.include <bsd.lib.mk>
