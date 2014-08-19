#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:30 tls Exp $

LIB=	lldbPluginUnwindAssemblyInstEmulation

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/UnwindAssembly/InstEmulation

SRCS+=	UnwindAssemblyInstEmulation.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
