#	$NetBSD: Makefile,v 1.1.2.2 2015/06/04 20:05:46 snj Exp $

LIB=	lldConfig

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/Config

SRCS+=	Version.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
