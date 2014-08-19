#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:28 tls Exp $

LIB=	lldbPluginCXXItaniumABI

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/LanguageRuntime/CPlusPlus/ItaniumABI

SRCS+=	ItaniumABILanguageRuntime.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
