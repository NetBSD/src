#	$NetBSD: Makefile,v 1.7.14.1 2019/06/10 21:50:55 christos Exp $

LIB=	LLVMipo

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/IPO

SRCS+=	AlwaysInliner.cpp \
	ArgumentPromotion.cpp \
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
	InferFunctionAttrs.cpp \
	Inliner.cpp \
	InlineSimple.cpp \
	Internalize.cpp \
	IPConstantPropagation.cpp \
	IPO.cpp \
	LoopExtractor.cpp \
	LowerTypeTests.cpp \
	MergeFunctions.cpp \
	PartialInlining.cpp \
	PassManagerBuilder.cpp \
	PruneEH.cpp \
	SampleProfile.cpp \
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
