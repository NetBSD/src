#	$NetBSD: Makefile,v 1.1 2019/03/10 12:14:05 mrg Exp $

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
	AMDGPUCallLowering.cpp \
	AMDGPUCodeGenPrepare.cpp \
	AMDGPUFrameLowering.cpp \
	AMDGPUHSAMetadataStreamer.cpp \
	AMDGPUInstrInfo.cpp \
	AMDGPUInstructionSelector.cpp \
	AMDGPUIntrinsicInfo.cpp \
	AMDGPUISelDAGToDAG.cpp \
	AMDGPUISelLowering.cpp \
	AMDGPULegalizerInfo.cpp \
	AMDGPULibCalls.cpp \
	AMDGPULibFunc.cpp \
	AMDGPULowerIntrinsics.cpp \
	AMDGPULowerKernelArguments.cpp \
	AMDGPULowerKernelAttributes.cpp \
	AMDGPUMachineCFGStructurizer.cpp \
	AMDGPUMachineFunction.cpp \
	AMDGPUMachineModuleInfo.cpp \
	AMDGPUMacroFusion.cpp \
	AMDGPUMCInstLower.cpp \
	AMDGPUOpenCLEnqueuedBlockLowering.cpp \
	AMDGPUPromoteAlloca.cpp \
	AMDGPURegAsmNames.inc.cpp \
	AMDGPURegisterBankInfo.cpp \
	AMDGPURegisterInfo.cpp \
	AMDGPURewriteOutArguments.cpp \
	AMDGPUSubtarget.cpp \
	AMDGPUTargetMachine.cpp \
	AMDGPUTargetObjectFile.cpp \
	AMDGPUTargetTransformInfo.cpp \
	AMDGPUUnifyDivergentExitNodes.cpp \
	AMDGPUUnifyMetadata.cpp \
	AMDGPUInline.cpp \
	AMDGPUPerfHintAnalysis.cpp \
	AMDILCFGStructurizer.cpp \
	GCNHazardRecognizer.cpp \
	GCNIterativeScheduler.cpp \
	GCNMinRegStrategy.cpp \
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
	SIDebuggerInsertNops.cpp \
	SIFixSGPRCopies.cpp \
	SIFixVGPRCopies.cpp \
	SIFixWWMLiveness.cpp \
	SIFoldOperands.cpp \
	SIFormMemoryClauses.cpp \
	SIFrameLowering.cpp \
	SIInsertSkips.cpp \
	SIInsertWaitcnts.cpp \
	SIInstrInfo.cpp \
	SIISelLowering.cpp \
	SILoadStoreOptimizer.cpp \
	SILowerControlFlow.cpp \
	SILowerI1Copies.cpp \
	SIMachineFunctionInfo.cpp \
	SIMachineScheduler.cpp \
	SIMemoryLegalizer.cpp \
	SIOptimizeExecMasking.cpp \
	SIOptimizeExecMaskingPreRA.cpp \
	SIPeepholeSDWA.cpp \
	SIRegisterInfo.cpp \
	SIShrinkInstructions.cpp \
	SIWholeQuadMode.cpp \
	GCNILPSched.cpp

TABLEGEN_SRC=		AMDGPU.td AMDGPUGISel.td R600.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/AMDGPU
TABLEGEN_OUTPUT.AMDGPU.td= \
	AMDGPUGenAsmMatcher.inc|-gen-asm-matcher \
	AMDGPUGenAsmWriter.inc|-gen-asm-writer \
	AMDGPUGenCallingConv.inc|-gen-callingconv \
	AMDGPUGenDAGISel.inc|-gen-dag-isel \
	AMDGPUGenDisassemblerTables.inc|-gen-disassembler \
	AMDGPUGenInstrInfo.inc|-gen-instr-info \
	AMDGPUGenIntrinsicEnums.inc|-gen-tgt-intrinsic-enums \
	AMDGPUGenIntrinsicImpl.inc|-gen-tgt-intrinsic-impl \
	AMDGPUGenMCCodeEmitter.inc|-gen-emitter \
	AMDGPUGenMCPseudoLowering.inc|-gen-pseudo-lowering \
	AMDGPUGenRegisterBank.inc|-gen-register-bank \
	AMDGPUGenRegisterInfo.inc|-gen-register-info \
	AMDGPUGenSearchableTables.inc|-gen-searchable-tables \
	AMDGPUGenSubtargetInfo.inc|-gen-subtarget

TABLEGEN_OUTPUT.AMDGPUGISel.td= \
	AMDGPUGenGlobalISel.inc|-gen-global-isel

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
