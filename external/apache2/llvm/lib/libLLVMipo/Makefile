#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:11 cjep Exp $

LIB=	LLVMipo

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/IPO

SRCS+=	AlwaysInliner.cpp \
	Annotation2Metadata.cpp \
	ArgumentPromotion.cpp \
	AttributorAttributes.cpp \
	Attributor.cpp \
	BarrierNoopPass.cpp \
	BlockExtractor.cpp \
	CalledValuePropagation.cpp \
	ConstantMerge.cpp \
	CrossDSOCFI.cpp \
	DeadArgumentElimination.cpp \
	ElimAvailExtern.cpp \
	ExtractGV.cpp \
	ForceFunctionAttrs.cpp \
	FunctionAttrs.cpp \
	FunctionImport.cpp \
	GlobalDCE.cpp \
	GlobalOpt.cpp \
	GlobalSplit.cpp \
	HotColdSplitting.cpp \
	InferFunctionAttrs.cpp \
	Inliner.cpp \
	InlineSimple.cpp \
	Internalize.cpp \
	IPO.cpp \
	IROutliner.cpp \
	LoopExtractor.cpp \
	LowerTypeTests.cpp \
	MergeFunctions.cpp \
	OpenMPOpt.cpp \
	PartialInlining.cpp \
	PassManagerBuilder.cpp \
	PruneEH.cpp \
	SampleContextTracker.cpp \
	SampleProfile.cpp \
	SampleProfileProbe.cpp \
	SCCP.cpp \
	StripDeadPrototypes.cpp \
	StripSymbols.cpp \
	SyntheticCountsPropagation.cpp \
	ThinLTOBitcodeWriter.cpp \
	WholeProgramDevirt.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
