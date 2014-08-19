#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:30 tls Exp $

LIB=	lldbPluginPlatformMacOSX

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Platform/MacOSX

SRCS+=	PlatformDarwin.cpp \
	PlatformDarwinKernel.cpp \
	PlatformiOSSimulator.cpp \
	PlatformMacOSX.cpp \
	PlatformRemoteiOS.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
