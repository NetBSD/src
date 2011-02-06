#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:01 joerg Exp $

LIB=	LLVMScalarOpts

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Scalar

SRCS+=	ADCE.cpp \
	BasicBlockPlacement.cpp \
	CodeGenPrepare.cpp \
	ConstantProp.cpp \
	CorrelatedValuePropagation.cpp \
	DCE.cpp \
	DeadStoreElimination.cpp \
	EarlyCSE.cpp \
	GEPSplitter.cpp \
	GVN.cpp \
	IndVarSimplify.cpp \
	JumpThreading.cpp \
	LICM.cpp \
	LoopDeletion.cpp \
	LoopIdiomRecognize.cpp \
	LoopInstSimplify.cpp \
	LoopRotation.cpp \
	LoopStrengthReduce.cpp \
	LoopUnrollPass.cpp \
	LoopUnswitch.cpp \
	LowerAtomic.cpp \
	MemCpyOptimizer.cpp \
	Reassociate.cpp \
	Reg2Mem.cpp \
	SCCP.cpp \
	Scalar.cpp \
	ScalarReplAggregates.cpp \
	SimplifyCFGPass.cpp \
	SimplifyHalfPowrLibCalls.cpp \
	SimplifyLibCalls.cpp \
	Sink.cpp \
	TailDuplication.cpp \
	TailRecursionElimination.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
