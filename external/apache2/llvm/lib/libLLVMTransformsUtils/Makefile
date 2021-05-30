#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:55 joerg Exp $

LIB=	LLVMTransformsUtils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Utils

SRCS+=	AddDiscriminators.cpp \
	AMDGPUEmitPrintf.cpp \
	ASanStackFrameLayout.cpp \
	AssumeBundleBuilder.cpp \
	AutoInitRemark.cpp \
	BasicBlockUtils.cpp \
	BreakCriticalEdges.cpp \
	BuildLibCalls.cpp \
	BypassSlowDivision.cpp \
	CallGraphUpdater.cpp \
	CallPromotionUtils.cpp \
	CanonicalizeAliases.cpp \
	CanonicalizeFreezeInLoops.cpp \
	CloneFunction.cpp \
	CloneModule.cpp \
	CodeExtractor.cpp \
	CodeMoverUtils.cpp \
	CtorUtils.cpp \
	Debugify.cpp \
	DemoteRegToStack.cpp \
	EntryExitInstrumenter.cpp \
	EscapeEnumerator.cpp \
	Evaluator.cpp \
	FixIrreducible.cpp \
	FlattenCFG.cpp \
	FunctionComparator.cpp \
	FunctionImportUtils.cpp \
	GlobalStatus.cpp \
	GuardUtils.cpp \
	HelloWorld.cpp \
	InjectTLIMappings.cpp \
	InlineFunction.cpp \
	InstructionNamer.cpp \
	IntegerDivision.cpp \
	LCSSA.cpp \
	LibCallsShrinkWrap.cpp \
	Local.cpp \
	LoopPeel.cpp \
	LoopRotationUtils.cpp \
	LoopSimplify.cpp \
	LoopUnrollAndJam.cpp \
	LoopUnroll.cpp \
	LoopUnrollRuntime.cpp \
	LoopUtils.cpp \
	LoopVersioning.cpp \
	LowerInvoke.cpp \
	LowerMemIntrinsics.cpp \
	LowerSwitch.cpp \
	MatrixUtils.cpp \
	Mem2Reg.cpp \
	MetaRenamer.cpp \
	ModuleUtils.cpp \
	NameAnonGlobals.cpp \
	PredicateInfo.cpp \
	PromoteMemoryToRegister.cpp \
	RelLookupTableConverter.cpp \
	SampleProfileLoaderBaseUtil.cpp \
	SanitizerStats.cpp \
	ScalarEvolutionExpander.cpp \
	SCCPSolver.cpp \
	SimplifyCFG.cpp \
	SimplifyIndVar.cpp \
	SimplifyLibCalls.cpp \
	SizeOpts.cpp \
	SplitModule.cpp \
	SSAUpdaterBulk.cpp \
	SSAUpdater.cpp \
	StripGCRelocates.cpp \
	StripNonLineTableDebugInfo.cpp \
	SymbolRewriter.cpp \
	UnifyFunctionExitNodes.cpp \
	UnifyLoopExits.cpp \
	Utils.cpp \
	ValueMapper.cpp \
	VNCoercion.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
