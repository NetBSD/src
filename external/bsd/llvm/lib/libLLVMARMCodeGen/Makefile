#	$NetBSD: Makefile,v 1.17.2.1 2018/07/28 04:37:10 pgoyette Exp $

LIB=	LLVMARMCodeGen

.include <bsd.init.mk>

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/ARM

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM

SRCS+=	ARMAsmPrinter.cpp \
	A15SDOptimizer.cpp \
	ARMBaseInstrInfo.cpp \
	ARMBaseRegisterInfo.cpp \
	ARMCallLowering.cpp \
	ARMComputeBlockSize.cpp \
	ARMConstantIslandPass.cpp \
	ARMConstantPoolValue.cpp \
	ARMExpandPseudoInsts.cpp \
	ARMFastISel.cpp \
	ARMFrameLowering.cpp \
	ARMHazardRecognizer.cpp \
	ARMInstrInfo.cpp \
	ARMInstructionSelector.cpp \
	ARMISelDAGToDAG.cpp \
	ARMISelLowering.cpp \
	ARMLegalizerInfo.cpp \
	ARMLoadStoreOptimizer.cpp \
	ARMMachineFunctionInfo.cpp \
	ARMMacroFusion.cpp \
	ARMMCInstLower.cpp \
	ARMOptimizeBarriersPass.cpp \
	ARMParallelDSP.cpp \
	ARMRegisterBankInfo.cpp \
	ARMRegisterInfo.cpp \
	ARMSelectionDAGInfo.cpp \
	ARMSubtarget.cpp \
	ARMTargetMachine.cpp \
	ARMTargetObjectFile.cpp \
	ARMTargetTransformInfo.cpp \
	MLxExpansionPass.cpp \
	Thumb1FrameLowering.cpp \
	Thumb1InstrInfo.cpp \
	Thumb2InstrInfo.cpp \
	Thumb2ITBlockPass.cpp \
	Thumb2SizeReduction.cpp \
	ThumbRegisterInfo.cpp

TABLEGEN_SRC=		ARM.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/ARM
TABLEGEN_OUTPUT= \
	ARMGenAsmMatcher.inc|-gen-asm-matcher \
	ARMGenAsmWriter.inc|-gen-asm-writer \
	ARMGenCallingConv.inc|-gen-callingconv \
	ARMGenCodeEmitter.inc|-gen-emitter \
	ARMGenDAGISel.inc|-gen-dag-isel \
	ARMGenDisassemblerTables.inc|-gen-disassembler \
	ARMGenFastISel.inc|-gen-fast-isel \
	ARMGenGlobalISel.inc|-gen-global-isel \
	ARMGenInstrInfo.inc|-gen-instr-info \
	ARMGenMCCodeEmitter.inc|-gen-emitter \
	ARMGenMCPseudoLowering.inc|-gen-pseudo-lowering \
	ARMGenRegisterBank.inc|-gen-register-bank \
	ARMGenRegisterInfo.inc|-gen-register-info \
	ARMGenSubtargetInfo.inc|-gen-subtarget \
	ARMGenSystemRegister.inc|-gen-searchable-tables


.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
