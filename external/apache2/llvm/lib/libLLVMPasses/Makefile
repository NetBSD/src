#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:09 cjep Exp $

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
