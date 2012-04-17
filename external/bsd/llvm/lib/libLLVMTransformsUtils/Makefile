#	$NetBSD: Makefile,v 1.4.2.2 2012/04/17 00:03:33 yamt Exp $

LIB=	LLVMTransformsUtils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Utils

SRCS+=	AddrModeMatcher.cpp \
	BasicBlockUtils.cpp \
	BreakCriticalEdges.cpp \
	BuildLibCalls.cpp \
	CloneFunction.cpp \
	CloneModule.cpp \
	CmpInstAnalysis.cpp \
	CodeExtractor.cpp \
	DemoteRegToStack.cpp \
	InlineFunction.cpp \
	InstructionNamer.cpp \
	LCSSA.cpp \
	Local.cpp \
	LoopSimplify.cpp \
	LoopUnroll.cpp \
	LoopUnrollRuntime.cpp \
	LowerExpectIntrinsic.cpp \
	LowerInvoke.cpp \
	LowerSwitch.cpp \
	Mem2Reg.cpp \
	ModuleUtils.cpp \
	PromoteMemoryToRegister.cpp \
	SSAUpdater.cpp \
	SimplifyCFG.cpp \
	SimplifyIndVar.cpp \
	SimplifyInstructions.cpp \
	UnifyFunctionExitNodes.cpp \
	Utils.cpp \
	ValueMapper.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
