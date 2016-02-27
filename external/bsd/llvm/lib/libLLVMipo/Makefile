#	$NetBSD: Makefile,v 1.6 2016/02/27 21:47:35 joerg Exp $

LIB=	LLVMipo

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/IPO

SRCS+=	ArgumentPromotion.cpp \
	BarrierNoopPass.cpp \
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
	IPConstantPropagation.cpp \
	IPO.cpp \
	InferFunctionAttrs.cpp \
	InlineAlways.cpp \
	InlineSimple.cpp \
	Inliner.cpp \
	Internalize.cpp \
	LoopExtractor.cpp \
	LowerBitSets.cpp \
	MergeFunctions.cpp \
	PartialInlining.cpp \
	PassManagerBuilder.cpp \
	PruneEH.cpp \
	SampleProfile.cpp \
	StripDeadPrototypes.cpp \
	StripSymbols.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
