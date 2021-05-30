#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:53 joerg Exp $

LIB=	LLVMOrc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Orc

SRCS+=	CompileOnDemandLayer.cpp \
	CompileUtils.cpp \
	Core.cpp \
	DebugObjectManagerPlugin.cpp \
	DebugUtils.cpp \
	ExecutionUtils.cpp \
	IndirectionUtils.cpp \
	IRCompileLayer.cpp \
	IRTransformLayer.cpp \
	JITTargetMachineBuilder.cpp \
	Layer.cpp \
	LazyReexports.cpp \
	LLJIT.cpp \
	MachOPlatform.cpp \
	Mangling.cpp \
	ObjectLinkingLayer.cpp \
	ObjectTransformLayer.cpp \
	OrcABISupport.cpp \
	OrcV2CBindings.cpp \
	RTDyldObjectLinkingLayer.cpp \
	SpeculateAnalyses.cpp \
	Speculation.cpp \
	TargetProcessControl.cpp \
	ThreadSafeModule.cpp \
	TPCDebugObjectRegistrar.cpp \
	TPCDynamicLibrarySearchGenerator.cpp \
	TPCEHFrameRegistrar.cpp \
	TPCIndirectionUtils.cpp

.include <bsd.lib.mk>
