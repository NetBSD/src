#	$NetBSD: Makefile,v 1.1.6.2 2014/08/19 23:51:28 tls Exp $

LIB=	lldbPluginABIMacOSX_arm64

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/ABI/MacOSX-arm64

SRCS+=	ABIMacOSX_arm64.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
