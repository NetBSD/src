#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:09 cjep Exp $

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
