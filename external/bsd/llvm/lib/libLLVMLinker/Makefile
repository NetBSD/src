#	$NetBSD: Makefile,v 1.1.6.2 2014/05/22 16:14:42 yamt Exp $

LIB=	LLVMLinker

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Linker

SRCS+=	LinkModules.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
