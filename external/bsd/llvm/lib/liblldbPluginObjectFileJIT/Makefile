#	$NetBSD: Makefile,v 1.1.2.2 2014/08/10 07:09:11 tls Exp $

LIB=	lldbPluginObjectFileJIT

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/ObjectFile/JIT

SRCS+=	ObjectFileJIT.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
