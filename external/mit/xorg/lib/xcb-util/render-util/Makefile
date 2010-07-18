#	$NetBSD: Makefile,v 1.1 2010/07/18 07:01:39 mrg Exp $

XCBUTIL=	render-util

SRCS=	cache.c glyph.c util.c

# xxx render-util vs renderutil
PKGCONFIG=	xcb-renderutil
.include "../xcb-util.mk"

LIBDPLIBS+=\
	xcb-render	${.CURDIR}/../../libxcb/render

# xxx render-util vs renderutil
CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/renderutil
.PATH: ${X11SRCDIR.xcb-util}/renderutil
