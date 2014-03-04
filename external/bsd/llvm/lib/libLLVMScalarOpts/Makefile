#	$NetBSD: Makefile,v 1.16 2014/03/04 21:05:06 joerg Exp $

LIB=	LLVMScalarOpts

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Scalar

SRCS+=	ADCE.cpp \
	ConstantHoisting.cpp \
	ConstantProp.cpp \
	CorrelatedValuePropagation.cpp \
	DCE.cpp \
	DeadStoreElimination.cpp \
	EarlyCSE.cpp \
	FlattenCFGPass.cpp \
	GlobalMerge.cpp \
	GVN.cpp \
	IndVarSimplify.cpp \
	JumpThreading.cpp \
	LICM.cpp \
	LoopDeletion.cpp \
	LoopIdiomRecognize.cpp \
	LoopInstSimplify.cpp \
	LoopRerollPass.cpp \
	LoopRotation.cpp \
	LoopStrengthReduce.cpp \
	LoopUnrollPass.cpp \
	LoopUnswitch.cpp \
	LowerAtomic.cpp \
	MemCpyOptimizer.cpp \
	PartiallyInlineLibCalls.cpp \
	Reassociate.cpp \
	Reg2Mem.cpp \
	SCCP.cpp \
	SROA.cpp \
	SampleProfile.cpp \
	Scalar.cpp \
	ScalarReplAggregates.cpp \
	Scalarizer.cpp \
	SimplifyCFGPass.cpp \
	Sink.cpp \
	StructurizeCFG.cpp \
	TailRecursionElimination.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
