#	$NetBSD: Makefile,v 1.2 2018/07/17 18:55:28 joerg Exp $

LIB=	LLVMPasses

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Passes

SRCS+=	PassBuilder.cpp \
	PassPlugin.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
