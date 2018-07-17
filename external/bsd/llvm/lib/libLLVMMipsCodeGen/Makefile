#	$NetBSD: Makefile,v 1.29 2018/07/17 18:55:28 joerg Exp $

LIB=	LLVMMipsCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips

SRCS+=	MicroMipsSizeReduction.cpp \
	Mips16FrameLowering.cpp \
	Mips16HardFloat.cpp \
	Mips16HardFloatInfo.cpp \
	Mips16InstrInfo.cpp \
	Mips16ISelDAGToDAG.cpp \
	Mips16ISelLowering.cpp \
	Mips16RegisterInfo.cpp \
	MipsAnalyzeImmediate.cpp \
	MipsAsmPrinter.cpp \
	MipsBranchExpansion.cpp \
	MipsCallLowering.cpp \
	MipsCCState.cpp \
	MipsConstantIslandPass.cpp \
	MipsDelaySlotFiller.cpp \
	MipsExpandPseudo.cpp \
	MipsFastISel.cpp \
	MipsFrameLowering.cpp \
	MipsInstrInfo.cpp \
	MipsInstructionSelector.cpp \
	MipsISelDAGToDAG.cpp \
	MipsISelLowering.cpp \
	MipsLegalizerInfo.cpp \
	MipsMachineFunction.cpp \
	MipsMCInstLower.cpp \
	MipsModuleISelDAGToDAG.cpp \
	MipsOptimizePICCall.cpp \
	MipsOs16.cpp \
	MipsRegisterBankInfo.cpp \
	MipsRegisterInfo.cpp \
	MipsSEFrameLowering.cpp \
	MipsSEInstrInfo.cpp \
	MipsSEISelDAGToDAG.cpp \
	MipsSEISelLowering.cpp \
	MipsSERegisterInfo.cpp \
	MipsSubtarget.cpp \
	MipsTargetMachine.cpp \
	MipsTargetObjectFile.cpp

TABLEGEN_SRC=		Mips.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/Mips
TABLEGEN_OUTPUT= \
	MipsGenAsmMatcher.inc|-gen-asm-matcher \
	MipsGenAsmWriter.inc|-gen-asm-writer \
	MipsGenCallingConv.inc|-gen-callingconv \
	MipsGenCodeEmitter.inc|-gen-emitter \
	MipsGenDAGISel.inc|-gen-dag-isel \
	MipsGenDisassemblerTables.inc|-gen-disassembler \
	MipsGenFastISel.inc|-gen-fast-isel \
	MipsGenGlobalISel.inc|-gen-global-isel \
	MipsGenInstrInfo.inc|-gen-instr-info \
	MipsGenMCCodeEmitter.inc|-gen-emitter \
	MipsGenMCPseudoLowering.inc|-gen-pseudo-lowering \
	MipsGenRegisterBank.inc|-gen-register-bank \
	MipsGenRegisterInfo.inc|-gen-register-info \
	MipsGenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
