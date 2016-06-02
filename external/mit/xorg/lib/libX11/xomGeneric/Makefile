# $NetBSD: Makefile,v 1.2 2016/06/02 22:41:27 mrg Exp $

LIBISMODULE=	yes
SHLIB_MAJOR=	2 # XXX
SHLIB_MINOR=	0

.include <bsd.own.mk>
.include <bsd.x11.mk>

LIB=	xomGeneric
LIBDIR=	${X11LIBDIR}/locale/lib/common
SRCS=	${SRCS.xomGeneric}

.include "../Makefile.xomGeneric"
.include <bsd.lib.mk>
