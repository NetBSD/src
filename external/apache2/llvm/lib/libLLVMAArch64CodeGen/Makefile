#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:50 joerg Exp $

LIB=	LLVMAArch64CodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/AArch64

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/AArch64

SRCS+=	AArch64A53Fix835769.cpp \
	AArch64A57FPLoadBalancing.cpp \
	AArch64AdvSIMDScalarPass.cpp \
	AArch64AsmPrinter.cpp \
	AArch64BranchTargets.cpp \
	AArch64CallingConvention.cpp \
	AArch64CleanupLocalDynamicTLSPass.cpp \
	AArch64CollectLOH.cpp \
	AArch64CompressJumpTables.cpp \
	AArch64CondBrTuning.cpp \
	AArch64ConditionalCompares.cpp \
	AArch64ConditionOptimizer.cpp \
	AArch64DeadRegisterDefinitionsPass.cpp \
	AArch64ExpandImm.cpp \
	AArch64ExpandPseudoInsts.cpp \
	AArch64FalkorHWPFFix.cpp \
	AArch64FastISel.cpp \
	AArch64FrameLowering.cpp \
	AArch64InstrInfo.cpp \
	AArch64ISelDAGToDAG.cpp \
	AArch64ISelLowering.cpp \
	AArch64LoadStoreOptimizer.cpp \
	AArch64LowerHomogeneousPrologEpilog.cpp \
	AArch64MachineFunctionInfo.cpp \
	AArch64MacroFusion.cpp \
	AArch64MCInstLower.cpp \
	AArch64PBQPRegAlloc.cpp \
	AArch64PromoteConstant.cpp \
	AArch64RedundantCopyElimination.cpp \
	AArch64RegisterInfo.cpp \
	AArch64SelectionDAGInfo.cpp \
	AArch64SIMDInstrOpt.cpp \
	AArch64SLSHardening.cpp \
	AArch64SpeculationHardening.cpp \
	AArch64StackTagging.cpp \
	AArch64StackTaggingPreRA.cpp \
	AArch64StorePairSuppress.cpp \
	AArch64Subtarget.cpp \
	AArch64TargetMachine.cpp \
	AArch64TargetObjectFile.cpp \
	AArch64TargetTransformInfo.cpp \
	SVEIntrinsicOpts.cpp

.PATH: ${LLVM_SRCDIR}/lib/Target/AArch64/GISel

SRCS+=	AArch64CallLowering.cpp \
	AArch64GlobalISelUtils.cpp \
	AArch64InstructionSelector.cpp \
	AArch64LegalizerInfo.cpp \
	AArch64O0PreLegalizerCombiner.cpp \
	AArch64PostLegalizerCombiner.cpp \
	AArch64PostLegalizerLowering.cpp \
	AArch64PostSelectOptimize.cpp \
	AArch64PreLegalizerCombiner.cpp \
	AArch64RegisterBankInfo.cpp

TABLEGEN_SRC=		AArch64.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/AArch64
TABLEGEN_OUTPUT= \
	AArch64GenAsmMatcher.inc|-gen-asm-matcher \
	AArch64GenAsmWriter1.inc|-gen-asm-writer^-asmwriternum=1 \
	AArch64GenAsmWriter.inc|-gen-asm-writer \
	AArch64GenCallingConv.inc|-gen-callingconv \
	AArch64GenDAGISel.inc|-gen-dag-isel \
	AArch64GenDisassemblerTables.inc|-gen-disassembler \
	AArch64GenExegesis.inc|-gen-exegesis \
	AArch64GenFastISel.inc|-gen-fast-isel \
	AArch64GenGICombiner.inc|-gen-global-isel-combiner^-combiners=AArch64PreLegalizerCombinerHelper \
	AArch64GenGlobalISel.inc|-gen-global-isel \
	AArch64GenInstrInfo.inc|-gen-instr-info \
	AArch64GenMCCodeEmitter.inc|-gen-emitter \
	AArch64GenMCPseudoLowering.inc|-gen-pseudo-lowering \
	AArch64GenO0PreLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=AArch64O0PreLegalizerCombinerHelper \
	AArch64GenPreLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=AArch64PreLegalizerCombinerHelper \
	AArch64GenPostLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=AArch64PostLegalizerCombinerHelper \
	AArch64GenPostLegalizeGILowering.inc|-gen-global-isel-combiner^-combiners=AArch64PostLegalizerLoweringHelper \
	AArch64GenRegisterBank.inc|-gen-register-bank \
	AArch64GenRegisterInfo.inc|-gen-register-info \
	AArch64GenSubtargetInfo.inc|-gen-subtarget \
	AArch64GenSystemOperands.inc|-gen-searchable-tables


.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
