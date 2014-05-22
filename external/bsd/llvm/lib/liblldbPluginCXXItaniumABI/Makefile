#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:48 yamt Exp $

LIB=	lldbPluginCXXItaniumABI

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/LanguageRuntime/CPlusPlus/ItaniumABI

SRCS+=	ItaniumABILanguageRuntime.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
