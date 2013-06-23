#	$NetBSD: Makefile,v 1.2.6.2 2013/06/23 06:28:23 tls Exp $

LIB=	LLVMLinker

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Linker

SRCS+=	LinkModules.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
