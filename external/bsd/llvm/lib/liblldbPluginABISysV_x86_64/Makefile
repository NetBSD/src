#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:28 tls Exp $

LIB=	lldbPluginABISysV_x86_64

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/ABI/SysV-x86_64

SRCS+=	ABISysV_x86_64.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
