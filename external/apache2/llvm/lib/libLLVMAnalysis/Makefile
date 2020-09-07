#	$NetBSD: Makefile,v 1.2 2020/09/07 00:18:25 mrg Exp $

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
	DDG.cpp \
	Delinearization.cpp \
	DemandedBits.cpp \
	DependenceAnalysis.cpp \
	DependenceGraphBuilder.cpp \
	DivergenceAnalysis.cpp \
	DominanceFrontier.cpp \
	DomPrinter.cpp \
	DomTreeUpdater.cpp \
	EHPersonalities.cpp \
	GlobalsModRef.cpp \
	GuardUtils.cpp \
	IndirectCallPromotionAnalysis.cpp \
	InlineCost.cpp \
	InstCount.cpp \
	InstructionPrecedenceTracking.cpp \
	InstructionSimplify.cpp \
	Interval.cpp \
	IntervalPartition.cpp \
	IVDescriptors.cpp \
	IVUsers.cpp \
	LazyBlockFrequencyInfo.cpp \
	LazyBranchProbabilityInfo.cpp \
	LazyCallGraph.cpp \
	LazyValueInfo.cpp \
	LegacyDivergenceAnalysis.cpp \
	Lint.cpp \
	Loads.cpp \
	LoopAccessAnalysis.cpp \
	LoopAnalysisManager.cpp \
	LoopCacheAnalysis.cpp \
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
	OrderedInstructions.cpp \
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
	StackSafetyAnalysis.cpp \
	SyncDependenceAnalysis.cpp \
	SyntheticCountsUtils.cpp \
	TargetLibraryInfo.cpp \
	TargetTransformInfo.cpp \
	Trace.cpp \
	TypeBasedAliasAnalysis.cpp \
	TypeMetadataUtils.cpp \
	VFABIDemangling.cpp \
	ValueLattice.cpp \
	ValueLatticeUtils.cpp \
	ValueTracking.cpp \
	VectorUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif

CWARNFLAGS.gcc+=	${GCC_NO_STRINGOP_OVERFLOW}
