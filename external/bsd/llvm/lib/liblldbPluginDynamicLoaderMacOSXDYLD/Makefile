#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:29 tls Exp $

LIB=	lldbPluginDynamicLoaderMacOSXDYLD

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/DynamicLoader/MacOSX-DYLD

SRCS+=	DynamicLoaderMacOSXDYLD.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
