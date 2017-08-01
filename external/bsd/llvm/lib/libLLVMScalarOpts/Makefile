#	$NetBSD: Makefile,v 1.23 2017/08/01 20:07:42 joerg Exp $

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
	GVNHoist.cpp \
	GVNSink.cpp \
	GuardWidening.cpp \
	IVUsersPrinter.cpp \
	IndVarSimplify.cpp \
	InductiveRangeCheckElimination.cpp \
	InferAddressSpaces.cpp \
	JumpThreading.cpp \
	LICM.cpp \
	LoopAccessAnalysisPrinter.cpp \
	LoopDataPrefetch.cpp \
	LoopDistribute.cpp \
	LoopDeletion.cpp \
	LoopIdiomRecognize.cpp \
	LoopInstSimplify.cpp \
	LoopInterchange.cpp \
	LoopLoadElimination.cpp \
	LoopPassManager.cpp \
	LoopPredication.cpp \
	LoopRerollPass.cpp \
	LoopRotation.cpp \
	LoopSimplifyCFG.cpp \
	LoopSink.cpp \
	LoopStrengthReduce.cpp \
	LoopUnrollPass.cpp \
	LoopUnswitch.cpp \
	LoopVersioningLICM.cpp \
	LowerAtomic.cpp \
	LowerExpectIntrinsic.cpp \
	LowerGuardIntrinsic.cpp \
	MemCpyOptimizer.cpp \
	MergedLoadStoreMotion.cpp \
	NaryReassociate.cpp \
	NewGVN.cpp \
	PartiallyInlineLibCalls.cpp \
	PlaceSafepoints.cpp \
	Reassociate.cpp \
	Reg2Mem.cpp \
	RewriteStatepointsForGC.cpp \
	SCCP.cpp \
	SROA.cpp \
	Scalar.cpp \
	Scalarizer.cpp \
	SeparateConstOffsetFromGEP.cpp \
	SimpleLoopUnswitch.cpp \
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
