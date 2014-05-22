#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:50 yamt Exp $

LIB=	lldbPluginSymbolVendorELF

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/SymbolVendor/ELF

SRCS+=	SymbolVendorELF.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
