#	$NetBSD: Makefile,v 1.1.4.2 2014/08/19 23:51:29 tls Exp $

LIB=	lldbPluginPlatformKalimba

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Platform/Kalimba

SRCS+=	PlatformKalimba.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
