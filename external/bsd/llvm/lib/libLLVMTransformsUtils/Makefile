#	$NetBSD: Makefile,v 1.20 2017/08/01 20:07:42 joerg Exp $

LIB=	LLVMTransformsUtils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Utils

SRCS+=	ASanStackFrameLayout.cpp \
	AddDiscriminators.cpp \
	BasicBlockUtils.cpp \
	BreakCriticalEdges.cpp \
	BuildLibCalls.cpp \
	BypassSlowDivision.cpp \
	CloneFunction.cpp \
	CloneModule.cpp \
	CmpInstAnalysis.cpp \
	CodeExtractor.cpp \
	CtorUtils.cpp \
	DemoteRegToStack.cpp \
	Evaluator.cpp \
	EscapeEnumerator.cpp \
	FlattenCFG.cpp \
	FunctionComparator.cpp \
	FunctionImportUtils.cpp \
	GlobalStatus.cpp \
	ImportedFunctionsInliningStatistics.cpp \
	InlineFunction.cpp \
	InstructionNamer.cpp \
	IntegerDivision.cpp \
	LCSSA.cpp \
	LibCallsShrinkWrap.cpp \
	Local.cpp \
	LoopSimplify.cpp \
	LoopUnroll.cpp \
	LoopUnrollPeel.cpp \
	LoopUnrollRuntime.cpp \
	LoopUtils.cpp \
	LoopVersioning.cpp \
	LowerInvoke.cpp \
	LowerMemIntrinsics.cpp \
	LowerSwitch.cpp \
	Mem2Reg.cpp \
	MetaRenamer.cpp \
	ModuleUtils.cpp \
	NameAnonGlobals.cpp \
	OrderedInstructions.cpp \
	PredicateInfo.cpp \
	PromoteMemoryToRegister.cpp \
	SSAUpdater.cpp \
	SanitizerStats.cpp \
	SimplifyCFG.cpp \
	SimplifyIndVar.cpp \
	SimplifyInstructions.cpp \
	SimplifyLibCalls.cpp \
	SplitModule.cpp \
	StripGCRelocates.cpp \
	StripNonLineTableDebugInfo.cpp \
	SymbolRewriter.cpp \
	UnifyFunctionExitNodes.cpp \
	Utils.cpp \
	VNCoercion.cpp \
	ValueMapper.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
