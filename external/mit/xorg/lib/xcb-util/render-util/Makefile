#	$NetBSD: Makefile,v 1.2 2011/08/11 23:15:39 joerg Exp $

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

CWARNFLAGS.clang+=	-Wno-tautological-compare
