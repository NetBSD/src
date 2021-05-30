#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:50 joerg Exp $

LIB=	LLVMAMDGPUCodeGen

.include <bsd.init.mk>

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/AMDGPU

.PATH: ${LLVM_SRCDIR}/lib/Target/AMDGPU

SRCS+=	AMDGPUAliasAnalysis.cpp \
	AMDGPUAlwaysInlinePass.cpp \
	AMDGPUAnnotateKernelFeatures.cpp \
	AMDGPUAnnotateUniformValues.cpp \
	AMDGPUArgumentUsageInfo.cpp \
	AMDGPUAsmPrinter.cpp \
	AMDGPUAtomicOptimizer.cpp \
	AMDGPUCallLowering.cpp \
	AMDGPUCodeGenPrepare.cpp \
	AMDGPUExportClustering.cpp \
	AMDGPUFixFunctionBitcasts.cpp \
	AMDGPUFrameLowering.cpp \
	AMDGPUGlobalISelUtils.cpp \
	AMDGPUHSAMetadataStreamer.cpp \
	AMDGPUInstCombineIntrinsic.cpp \
	AMDGPUInstrInfo.cpp \
	AMDGPUInstructionSelector.cpp \
	AMDGPUISelDAGToDAG.cpp \
	AMDGPUISelLowering.cpp \
	AMDGPULateCodeGenPrepare.cpp \
	AMDGPULegalizerInfo.cpp \
	AMDGPULibCalls.cpp \
	AMDGPULibFunc.cpp \
	AMDGPULowerIntrinsics.cpp \
	AMDGPULowerKernelArguments.cpp \
	AMDGPULowerKernelAttributes.cpp \
	AMDGPULowerModuleLDSPass.cpp \
	AMDGPUMachineCFGStructurizer.cpp \
	AMDGPUMachineFunction.cpp \
	AMDGPUMachineModuleInfo.cpp \
	AMDGPUMacroFusion.cpp \
	AMDGPUMCInstLower.cpp \
	AMDGPUMIRFormatter.cpp \
	AMDGPUOpenCLEnqueuedBlockLowering.cpp \
	AMDGPUPerfHintAnalysis.cpp \
	AMDGPUPostLegalizerCombiner.cpp \
	AMDGPUPreLegalizerCombiner.cpp \
	AMDGPUPrintfRuntimeBinding.cpp \
	AMDGPUPromoteAlloca.cpp \
	AMDGPUPropagateAttributes.cpp \
	AMDGPURegBankCombiner.cpp \
	AMDGPURegisterBankInfo.cpp \
	AMDGPURewriteOutArguments.cpp \
	AMDGPUSubtarget.cpp \
	AMDGPUTargetMachine.cpp \
	AMDGPUTargetObjectFile.cpp \
	AMDGPUTargetTransformInfo.cpp \
	AMDGPUUnifyDivergentExitNodes.cpp \
	AMDGPUUnifyMetadata.cpp \
	AMDILCFGStructurizer.cpp \
	GCNDPPCombine.cpp \
	GCNHazardRecognizer.cpp \
	GCNILPSched.cpp \
	GCNIterativeScheduler.cpp \
	GCNMinRegStrategy.cpp \
	GCNNSAReassign.cpp \
	GCNRegPressure.cpp \
	GCNSchedStrategy.cpp \
	R600AsmPrinter.cpp \
	R600ClauseMergePass.cpp \
	R600ControlFlowFinalizer.cpp \
	R600EmitClauseMarkers.cpp \
	R600ExpandSpecialInstrs.cpp \
	R600FrameLowering.cpp \
	R600InstrInfo.cpp \
	R600ISelLowering.cpp \
	R600MachineFunctionInfo.cpp \
	R600MachineScheduler.cpp \
	R600OpenCLImageTypeLoweringPass.cpp \
	R600OptimizeVectorRegisters.cpp \
	R600Packetizer.cpp \
	R600RegisterInfo.cpp \
	SIAnnotateControlFlow.cpp \
	SIFixSGPRCopies.cpp \
	SIFixVGPRCopies.cpp \
	SIFoldOperands.cpp \
	SIFormMemoryClauses.cpp \
	SIFrameLowering.cpp \
	SIInsertHardClauses.cpp \
	SIInsertWaitcnts.cpp \
	SIInstrInfo.cpp \
	SIISelLowering.cpp \
	SILateBranchLowering.cpp \
	SILoadStoreOptimizer.cpp \
	SILowerControlFlow.cpp \
	SILowerI1Copies.cpp \
	SILowerSGPRSpills.cpp \
	SIMachineFunctionInfo.cpp \
	SIMachineScheduler.cpp \
	SIMemoryLegalizer.cpp \
	SIModeRegister.cpp \
	SIOptimizeExecMasking.cpp \
	SIOptimizeExecMaskingPreRA.cpp \
	SIPeepholeSDWA.cpp \
	SIPostRABundler.cpp \
	SIPreAllocateWWMRegs.cpp \
	SIPreEmitPeephole.cpp \
	SIProgramInfo.cpp \
	SIRegisterInfo.cpp \
	SIShrinkInstructions.cpp \
	SIWholeQuadMode.cpp

TABLEGEN_SRC=		AMDGPU.td AMDGPUGISel.td InstCombineTables.td R600.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/AMDGPU
TABLEGEN_OUTPUT.AMDGPU.td= \
	AMDGPUGenAsmMatcher.inc|-gen-asm-matcher \
	AMDGPUGenAsmWriter.inc|-gen-asm-writer \
	AMDGPUGenCallingConv.inc|-gen-callingconv \
	AMDGPUGenDAGISel.inc|-gen-dag-isel \
	AMDGPUGenDisassemblerTables.inc|-gen-disassembler \
	AMDGPUGenInstrInfo.inc|-gen-instr-info \
	AMDGPUGenMCCodeEmitter.inc|-gen-emitter \
	AMDGPUGenMCPseudoLowering.inc|-gen-pseudo-lowering \
	AMDGPUGenRegisterBank.inc|-gen-register-bank \
	AMDGPUGenRegisterInfo.inc|-gen-register-info \
	AMDGPUGenSearchableTables.inc|-gen-searchable-tables \
	AMDGPUGenSubtargetInfo.inc|-gen-subtarget

TABLEGEN_OUTPUT.InstCombineTables.td= \
	InstCombineTables.inc|-gen-searchable-tables

TABLEGEN_OUTPUT.AMDGPUGISel.td= \
	AMDGPUGenGICombiner.inc|-gen-global-isel-combiner^-combiners=AMDGPUPreLegalizerCombinerHelper \
	AMDGPUGenGlobalISel.inc|-gen-global-isel \
	AMDGPUGenPostLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=AMDGPUPostLegalizerCombinerHelper \
	AMDGPUGenPreLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=AMDGPUPreLegalizerCombinerHelper \
        AMDGPUGenRegBankGICombiner.inc|-gen-global-isel-combiner^-combiners=AMDGPURegBankCombinerHelper

TABLEGEN_OUTPUT.R600.td= \
	R600GenAsmWriter.inc|-gen-asm-writer \
	R600GenCallingConv.inc|-gen-callingconv \
	R600GenDAGISel.inc|-gen-dag-isel \
	R600GenDFAPacketizer.inc|-gen-dfa-packetizer \
	R600GenInstrInfo.inc|-gen-instr-info \
	R600GenMCCodeEmitter.inc|-gen-emitter \
	R600GenRegisterInfo.inc|-gen-register-info \
	R600GenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
