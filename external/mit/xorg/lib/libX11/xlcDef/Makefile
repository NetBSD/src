# $NetBSD: Makefile,v 1.2 2016/06/02 22:41:26 mrg Exp $

LIBISMODULE=	yes
SHLIB_MAJOR=	2 # XXX
SHLIB_MINOR=	0

.include <bsd.own.mk>
.include <bsd.x11.mk>

LIB=	xlcDef
LIBDIR=	${X11LIBDIR}/locale/lib/common
SRCS=	${SRCS.xlcDef}

.include "../Makefile.xlcDef"
.include <bsd.lib.mk>
