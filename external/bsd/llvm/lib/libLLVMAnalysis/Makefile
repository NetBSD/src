#	$NetBSD: Makefile,v 1.17.2.1 2014/08/10 07:09:09 tls Exp $

LIB=	LLVMAnalysis

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Analysis

SRCS+=	AliasAnalysis.cpp \
	AliasAnalysisCounter.cpp \
	AliasAnalysisEvaluator.cpp \
	AliasDebugger.cpp \
	AliasSetTracker.cpp \
	Analysis.cpp \
	BasicAliasAnalysis.cpp \
	BlockFrequencyInfo.cpp \
	BlockFrequencyInfoImpl.cpp \
	BranchProbabilityInfo.cpp \
	CFG.cpp \
	CFGPrinter.cpp \
	CGSCCPassManager.cpp \
	CaptureTracking.cpp \
	CodeMetrics.cpp \
	ConstantFolding.cpp \
	CostModel.cpp \
	Delinearization.cpp \
	DependenceAnalysis.cpp \
	DomPrinter.cpp \
	DominanceFrontier.cpp \
	IVUsers.cpp \
	InstCount.cpp \
	InstructionSimplify.cpp \
	Interval.cpp \
	IntervalPartition.cpp \
	LazyCallGraph.cpp \
	LazyValueInfo.cpp \
	LibCallAliasAnalysis.cpp \
	LibCallSemantics.cpp \
	Lint.cpp \
	Loads.cpp \
	LoopInfo.cpp \
	LoopPass.cpp \
	MemDepPrinter.cpp \
	MemoryBuiltins.cpp \
	MemoryDependenceAnalysis.cpp \
	ModuleDebugInfoPrinter.cpp \
	NoAliasAnalysis.cpp \
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
	SparsePropagation.cpp \
	TargetTransformInfo.cpp \
	Trace.cpp \
	TypeBasedAliasAnalysis.cpp \
	ValueTracking.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
