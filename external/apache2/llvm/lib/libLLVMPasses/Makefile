#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:10 joerg Exp $

LIB=	LLVMPasses

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Passes

SRCS+=	PassBuilder.cpp \
	PassPlugin.cpp \
	StandardInstrumentations.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
