#	$NetBSD: Makefile,v 1.20 2016/02/27 21:47:34 joerg Exp $

LIB=	LLVMScalarOpts

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Scalar

SRCS+=	ADCE.cpp \
	AlignmentFromAssumptions.cpp \
	BDCE.cpp \
	ConstantHoisting.cpp \
	ConstantProp.cpp \
	CorrelatedValuePropagation.cpp \
	DCE.cpp \
	DeadStoreElimination.cpp \
	EarlyCSE.cpp \
	FlattenCFGPass.cpp \
	Float2Int.cpp \
	GVN.cpp \
	IndVarSimplify.cpp \
	InductiveRangeCheckElimination.cpp \
	JumpThreading.cpp \
	LICM.cpp \
	LoadCombine.cpp \
	LoopDistribute.cpp \
	LoopDeletion.cpp \
	LoopIdiomRecognize.cpp \
	LoopInstSimplify.cpp \
	LoopInterchange.cpp \
	LoopLoadElimination.cpp \
	LoopRerollPass.cpp \
	LoopRotation.cpp \
	LoopStrengthReduce.cpp \
	LoopUnrollPass.cpp \
	LoopUnswitch.cpp \
	LowerAtomic.cpp \
	LowerExpectIntrinsic.cpp \
	MemCpyOptimizer.cpp \
	MergedLoadStoreMotion.cpp \
	NaryReassociate.cpp \
	PartiallyInlineLibCalls.cpp \
	PlaceSafepoints.cpp \
	Reassociate.cpp \
	Reg2Mem.cpp \
	RewriteStatepointsForGC.cpp \
	SCCP.cpp \
	SROA.cpp \
	Scalar.cpp \
	ScalarReplAggregates.cpp \
	Scalarizer.cpp \
	SeparateConstOffsetFromGEP.cpp \
	SimplifyCFGPass.cpp \
	Sink.cpp \
	SpeculativeExecution.cpp \
	StraightLineStrengthReduce.cpp \
	StructurizeCFG.cpp \
	TailRecursionElimination.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
