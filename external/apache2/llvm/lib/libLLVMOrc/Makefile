#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:10 joerg Exp $

LIB=	LLVMOrc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Orc

SRCS+=	CompileOnDemandLayer.cpp \
	CompileUtils.cpp \
	Core.cpp \
	ExecutionUtils.cpp \
	IndirectionUtils.cpp \
	IRCompileLayer.cpp \
	IRTransformLayer.cpp \
	JITTargetMachineBuilder.cpp \
	Layer.cpp \
	LazyReexports.cpp \
	Legacy.cpp \
	LLJIT.cpp \
	NullResolver.cpp \
	ObjectLinkingLayer.cpp \
	ObjectTransformLayer.cpp \
	OrcABISupport.cpp \
	OrcCBindings.cpp \
	OrcError.cpp \
	OrcMCJITReplacement.cpp \
	RPCUtils.cpp \
	RTDyldObjectLinkingLayer.cpp \
	SpeculateAnalyses.cpp \
	Speculation.cpp \
	ThreadSafeModule.cpp

.include <bsd.lib.mk>
