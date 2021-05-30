#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:54 joerg Exp $

LIB=	LLVMPasses

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Passes

SRCS+=	PassBuilder.cpp \
	PassBuilderBindings.cpp \
	PassPlugin.cpp \
	StandardInstrumentations.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
