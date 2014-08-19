#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:30 tls Exp $

LIB=	lldbPluginPlatformNetBSD

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Platform/FreeBSD

SRCS+=	PlatformFreeBSD.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
