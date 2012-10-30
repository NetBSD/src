#	$NetBSD: Makefile,v 1.4.2.3 2012/10/30 18:55:52 yamt Exp $

LIB=	LLVMTransformsUtils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Utils

SRCS+=	AddrModeMatcher.cpp \
	BasicBlockUtils.cpp \
	BreakCriticalEdges.cpp \
	BuildLibCalls.cpp \
	BypassSlowDivision.cpp \
	CloneFunction.cpp \
	CloneModule.cpp \
	CmpInstAnalysis.cpp \
	CodeExtractor.cpp \
	DemoteRegToStack.cpp \
	InlineFunction.cpp \
	InstructionNamer.cpp \
	IntegerDivision.cpp \
	LCSSA.cpp \
	Local.cpp \
	LoopSimplify.cpp \
	LoopUnroll.cpp \
	LoopUnrollRuntime.cpp \
	LowerExpectIntrinsic.cpp \
	LowerInvoke.cpp \
	LowerSwitch.cpp \
	Mem2Reg.cpp \
	MetaRenamer.cpp \
	ModuleUtils.cpp \
	PromoteMemoryToRegister.cpp \
	SSAUpdater.cpp \
	SimplifyCFG.cpp \
	SimplifyIndVar.cpp \
	SimplifyInstructions.cpp \
	SimplifyLibCalls.cpp \
	UnifyFunctionExitNodes.cpp \
	Utils.cpp \
	ValueMapper.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
