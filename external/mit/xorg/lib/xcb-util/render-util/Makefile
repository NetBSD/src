#	$NetBSD: Makefile,v 1.2.2.1 2012/04/17 00:04:50 yamt Exp $

XCBUTIL=	render-util

SRCS=	cache.c glyph.c util.c

XCBUTIL_EXTRA_DPLIBS+=\
	xcb-render	${.CURDIR}/../../libxcb/render

# xxx render-util vs renderutil
PKGCONFIG=	xcb-renderutil
.include "../xcb-util.mk"

# xxx render-util vs renderutil
CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/renderutil
.PATH: ${X11SRCDIR.xcb-util}/renderutil

CWARNFLAGS.clang+=	-Wno-tautological-compare
