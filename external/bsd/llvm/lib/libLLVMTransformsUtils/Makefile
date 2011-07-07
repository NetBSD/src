#	$NetBSD: Makefile,v 1.3 2011/07/07 21:59:15 joerg Exp $

LIB=	LLVMTransformsUtils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Utils

SRCS+=	AddrModeMatcher.cpp \
	BasicBlockUtils.cpp \
	BasicInliner.cpp \
	BreakCriticalEdges.cpp \
	BuildLibCalls.cpp \
	CloneFunction.cpp \
	CloneModule.cpp \
	CodeExtractor.cpp \
	DemoteRegToStack.cpp \
	InlineFunction.cpp \
	InstructionNamer.cpp \
	LCSSA.cpp \
	Local.cpp \
	LoopSimplify.cpp \
	LoopUnroll.cpp \
	LowerExpectIntrinsic.cpp \
	LowerInvoke.cpp \
	LowerSwitch.cpp \
	Mem2Reg.cpp \
	PromoteMemoryToRegister.cpp \
	SSAUpdater.cpp \
	SimplifyCFG.cpp \
	SimplifyInstructions.cpp \
	UnifyFunctionExitNodes.cpp \
	Utils.cpp \
	ValueMapper.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
