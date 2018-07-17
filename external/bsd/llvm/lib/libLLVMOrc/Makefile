#	$NetBSD: Makefile,v 1.4 2018/07/17 18:55:28 joerg Exp $

LIB=	LLVMOrc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Orc

SRCS+=	CompileOnDemandLayer.cpp \
	Core.cpp \
	ExecutionUtils.cpp \
	IndirectionUtils.cpp \
	IRCompileLayer.cpp \
	IRTransformLayer.cpp \
	Layer.cpp \
	Legacy.cpp \
	LLJIT.cpp \
	NullResolver.cpp \
	ObjectTransformLayer.cpp \
	OrcABISupport.cpp \
	OrcCBindings.cpp \
	OrcError.cpp \
	OrcMCJITReplacement.cpp \
	RPCUtils.cpp \
	RTDyldObjectLinkingLayer.cpp

.include <bsd.lib.mk>
