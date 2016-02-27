#	$NetBSD: Makefile,v 1.21 2016/02/27 21:47:32 joerg Exp $

LIB=	LLVMAnalysis

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Analysis

SRCS+=	AliasAnalysis.cpp \
	AliasAnalysisEvaluator.cpp \
	AliasSetTracker.cpp \
	Analysis.cpp \
	AssumptionCache.cpp \
	BasicAliasAnalysis.cpp \
	BlockFrequencyInfo.cpp \
	BlockFrequencyInfoImpl.cpp \
	BranchProbabilityInfo.cpp \
	CFG.cpp \
	CFGPrinter.cpp \
	CFLAliasAnalysis.cpp \
	CGSCCPassManager.cpp \
	CallGraphSCCPass.cpp \
	CallGraph.cpp \
	CallPrinter.cpp \
	CaptureTracking.cpp \
	CodeMetrics.cpp \
	ConstantFolding.cpp \
	CostModel.cpp \
	Delinearization.cpp \
	DemandedBits.cpp \
	DependenceAnalysis.cpp \
	DivergenceAnalysis.cpp \
	DomPrinter.cpp \
	DominanceFrontier.cpp \
	EHPersonalities.cpp \
	GlobalsModRef.cpp \
	IVUsers.cpp \
	InlineCost.cpp \
	InstCount.cpp \
	InstructionSimplify.cpp \
	Interval.cpp \
	IntervalPartition.cpp \
	IteratedDominanceFrontier.cpp \
	LazyCallGraph.cpp \
	LazyValueInfo.cpp \
	Lint.cpp \
	Loads.cpp \
	LoopAccessAnalysis.cpp \
	LoopInfo.cpp \
	LoopPass.cpp \
	MemDepPrinter.cpp \
	MemDerefPrinter.cpp \
	MemoryBuiltins.cpp \
	MemoryDependenceAnalysis.cpp \
	MemoryLocation.cpp \
	ModuleDebugInfoPrinter.cpp \
	ObjCARCAliasAnalysis.cpp \
	ObjCARCAnalysisUtils.cpp \
	ObjCARCInstKind.cpp \
	OrderedBasicBlock.cpp \
	PHITransAddr.cpp \
	PostDominators.cpp \
	PtrUseVisitor.cpp \
	RegionInfo.cpp \
	RegionPass.cpp \
	RegionPrinter.cpp \
	ScalarEvolution.cpp \
	ScalarEvolutionAliasAnalysis.cpp \
	ScalarEvolutionExpander.cpp \
	ScalarEvolutionNormalization.cpp \
	ScopedNoAliasAA.cpp \
	SparsePropagation.cpp \
	TargetLibraryInfo.cpp \
	TargetTransformInfo.cpp \
	Trace.cpp \
	TypeBasedAliasAnalysis.cpp \
	ValueTracking.cpp \
	VectorUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
