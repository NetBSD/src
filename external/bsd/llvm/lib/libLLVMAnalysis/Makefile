#	$NetBSD: Makefile,v 1.24.2.1 2018/07/28 04:37:10 pgoyette Exp $

LIB=	LLVMAnalysis

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Analysis

SRCS+=	AliasAnalysis.cpp \
	AliasAnalysisEvaluator.cpp \
	AliasAnalysisSummary.cpp \
	AliasSetTracker.cpp \
	Analysis.cpp \
	AssumptionCache.cpp \
	BasicAliasAnalysis.cpp \
	BlockFrequencyInfo.cpp \
	BlockFrequencyInfoImpl.cpp \
	BranchProbabilityInfo.cpp \
	CallGraph.cpp \
	CallGraphSCCPass.cpp \
	CallPrinter.cpp \
	CaptureTracking.cpp \
	CFG.cpp \
	CFGPrinter.cpp \
	CFLAndersAliasAnalysis.cpp \
	CFLSteensAliasAnalysis.cpp \
	CGSCCPassManager.cpp \
	CmpInstAnalysis.cpp \
	CodeMetrics.cpp \
	ConstantFolding.cpp \
	CostModel.cpp \
	Delinearization.cpp \
	DemandedBits.cpp \
	DependenceAnalysis.cpp \
	DivergenceAnalysis.cpp \
	DominanceFrontier.cpp \
	DomPrinter.cpp \
	EHPersonalities.cpp \
	GlobalsModRef.cpp \
	IndirectCallPromotionAnalysis.cpp \
	InlineCost.cpp \
	InstCount.cpp \
	InstructionSimplify.cpp \
	Interval.cpp \
	IntervalPartition.cpp \
	IteratedDominanceFrontier.cpp \
	IVUsers.cpp \
	LazyBlockFrequencyInfo.cpp \
	LazyBranchProbabilityInfo.cpp \
	LazyCallGraph.cpp \
	LazyValueInfo.cpp \
	Lint.cpp \
	Loads.cpp \
	LoopAccessAnalysis.cpp \
	LoopAnalysisManager.cpp \
	LoopInfo.cpp \
	LoopPass.cpp \
	LoopUnrollAnalyzer.cpp \
	MemDepPrinter.cpp \
	MemDerefPrinter.cpp \
	MemoryBuiltins.cpp \
	MemoryDependenceAnalysis.cpp \
	MemoryLocation.cpp \
	MemorySSA.cpp \
	MemorySSAUpdater.cpp \
	ModuleDebugInfoPrinter.cpp \
	ModuleSummaryAnalysis.cpp \
	MustExecute.cpp \
	ObjCARCAliasAnalysis.cpp \
	ObjCARCAnalysisUtils.cpp \
	ObjCARCInstKind.cpp \
	OptimizationRemarkEmitter.cpp \
	OrderedBasicBlock.cpp \
	PHITransAddr.cpp \
	PhiValues.cpp \
	PostDominators.cpp \
	ProfileSummaryInfo.cpp \
	PtrUseVisitor.cpp \
	RegionInfo.cpp \
	RegionPass.cpp \
	RegionPrinter.cpp \
	ScalarEvolutionAliasAnalysis.cpp \
	ScalarEvolution.cpp \
	ScalarEvolutionExpander.cpp \
	ScalarEvolutionNormalization.cpp \
	ScopedNoAliasAA.cpp \
	SyntheticCountsUtils.cpp \
	TargetLibraryInfo.cpp \
	TargetTransformInfo.cpp \
	Trace.cpp \
	TypeBasedAliasAnalysis.cpp \
	TypeMetadataUtils.cpp \
	ValueLattice.cpp \
	ValueLatticeUtils.cpp \
	ValueTracking.cpp \
	VectorUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
