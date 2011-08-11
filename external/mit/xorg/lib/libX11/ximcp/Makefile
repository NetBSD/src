# $NetBSD: Makefile,v 1.2 2011/08/11 23:15:38 joerg Exp $

LIBISMODULE=	yes
SHLIB_MAJOR=	2 # XXX
SHLIB_MINOR=	0

.include <bsd.own.mk>
.include <bsd.x11.mk>

LIB=	ximcp
LIBDIR=	${X11LIBDIR}/locale/lib/common
SRCS=	${SRCS.ximcp}

CWARNFLAGS.clang+=	-Wno-unused

.include "../Makefile.ximcp"
