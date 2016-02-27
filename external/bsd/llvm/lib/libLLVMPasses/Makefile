#	$NetBSD: Makefile,v 1.1 2016/02/27 21:47:34 joerg Exp $

LIB=	LLVMPasses

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Passes

SRCS+=	PassBuilder.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
