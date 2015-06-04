#	$NetBSD: Makefile,v 1.1.2.2 2015/06/04 20:05:47 snj Exp $

LIB=	lldbPluginABISysV_ppc

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/ABI/SysV-ppc

SRCS+=	ABISysV_ppc.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
