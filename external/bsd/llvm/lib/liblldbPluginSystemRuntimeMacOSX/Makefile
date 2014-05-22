#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:50 yamt Exp $

LIB=	lldbPluginSystemRuntimeMacOSX

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/SystemRuntime/MacOSX

SRCS+=	SystemRuntimeMacOSX.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
