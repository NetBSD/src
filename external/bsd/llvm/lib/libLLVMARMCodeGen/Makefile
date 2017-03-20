#	$NetBSD: Makefile,v 1.15.2.1 2017/03/20 06:55:59 pgoyette Exp $

LIB=	LLVMARMCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/ARM

# Used only with GlobalISel:
#	ARMCallLowering.cpp
#	ARMInstructionSelector.cpp
#	ARMLegalizerInfo.cpp
#	ARMRegisterBankInfo.cpp

SRCS+=	ARMAsmPrinter.cpp \
	ARMBaseInstrInfo.cpp \
	ARMBaseRegisterInfo.cpp \
	ARMComputeBlockSize.cpp \
	ARMConstantIslandPass.cpp \
	ARMConstantPoolValue.cpp \
	ARMExpandPseudoInsts.cpp \
	ARMFastISel.cpp \
	ARMFrameLowering.cpp \
	ARMHazardRecognizer.cpp \
	ARMISelDAGToDAG.cpp \
	ARMISelLowering.cpp \
	ARMInstrInfo.cpp \
	ARMLoadStoreOptimizer.cpp \
	ARMMCInstLower.cpp \
	ARMMachineFunctionInfo.cpp \
	ARMOptimizeBarriersPass.cpp \
	ARMRegisterInfo.cpp \
	ARMSelectionDAGInfo.cpp \
	ARMSubtarget.cpp \
	ARMTargetMachine.cpp \
	ARMTargetObjectFile.cpp \
	ARMTargetTransformInfo.cpp \
	A15SDOptimizer.cpp \
	MLxExpansionPass.cpp \
	ThumbRegisterInfo.cpp \
	Thumb1InstrInfo.cpp \
	Thumb1FrameLowering.cpp \
	Thumb2ITBlockPass.cpp \
	Thumb2InstrInfo.cpp \
	Thumb2SizeReduction.cpp

TABLEGEN_SRC=		ARM.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/ARM
TABLEGEN_OUTPUT= \
	ARMGenRegisterInfo.inc|-gen-register-info \
	ARMGenInstrInfo.inc|-gen-instr-info \
	ARMGenCodeEmitter.inc|-gen-emitter \
	ARMGenMCCodeEmitter.inc|-gen-emitter \
	ARMGenMCPseudoLowering.inc|-gen-pseudo-lowering \
	ARMGenAsmWriter.inc|-gen-asm-writer \
	ARMGenAsmMatcher.inc|-gen-asm-matcher \
	ARMGenDAGISel.inc|-gen-dag-isel \
	ARMGenFastISel.inc|-gen-fast-isel \
	ARMGenCallingConv.inc|-gen-callingconv \
	ARMGenSubtargetInfo.inc|-gen-subtarget \
	ARMGenDisassemblerTables.inc|-gen-disassembler

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
