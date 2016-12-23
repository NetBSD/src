# $NetBSD: Makefile,v 1.3.6.1 2016/12/23 08:04:04 snj Exp $

LIBISMODULE=	yes
SHLIB_MAJOR=	2 # XXX
SHLIB_MINOR=	0

.include <bsd.own.mk>
.include <bsd.x11.mk>

LIB=	ximcp
LIBDIR=	${X11LIBDIR}/locale/lib/common
SRCS=	${SRCS.ximcp}
CFLAGS+=-I${X11SRCDIR.X11}/src

CWARNFLAGS.clang+=	-Wno-unused

.include "../Makefile.ximcp"
.include <bsd.lib.mk>
