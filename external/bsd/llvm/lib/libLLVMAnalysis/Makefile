#	$NetBSD: Makefile,v 1.5.2.3 2013/01/16 05:27:59 yamt Exp $

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
	BranchProbabilityInfo.cpp \
	CFGPrinter.cpp \
	CaptureTracking.cpp \
	CodeMetrics.cpp \
	ConstantFolding.cpp \
	CostModel.cpp \
	DbgInfoPrinter.cpp \
	DependenceAnalysis.cpp \
	DomPrinter.cpp \
	DominanceFrontier.cpp \
	IVUsers.cpp \
	InlineCost.cpp \
	InstCount.cpp \
	InstructionSimplify.cpp \
	Interval.cpp \
	IntervalPartition.cpp \
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
	PathNumbering.cpp \
	PathProfileInfo.cpp \
	PathProfileVerifier.cpp \
	PHITransAddr.cpp \
	PostDominators.cpp \
	ProfileDataLoaderPass.cpp \
	ProfileDataLoader.cpp \
	ProfileEstimatorPass.cpp \
	ProfileInfo.cpp \
	ProfileInfoLoader.cpp \
	ProfileInfoLoaderPass.cpp \
	ProfileVerifierPass.cpp \
	RegionInfo.cpp \
	RegionPass.cpp \
	RegionPrinter.cpp \
	ScalarEvolution.cpp \
	ScalarEvolutionAliasAnalysis.cpp \
	ScalarEvolutionExpander.cpp \
	ScalarEvolutionNormalization.cpp \
	SparsePropagation.cpp \
	Trace.cpp \
	TypeBasedAliasAnalysis.cpp \
	ValueTracking.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
