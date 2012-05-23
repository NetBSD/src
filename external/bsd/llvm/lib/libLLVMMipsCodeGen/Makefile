#	$NetBSD: Makefile,v 1.4.2.3 2012/05/23 10:07:26 yamt Exp $

LIB=	LLVMMipsCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips

SRCS+=	MipsAnalyzeImmediate.cpp \
	MipsAsmPrinter.cpp \
	MipsCodeEmitter.cpp \
	MipsDelaySlotFiller.cpp \
	MipsEmitGPRestore.cpp \
	MipsExpandPseudo.cpp \
	MipsFrameLowering.cpp \
	MipsInstrInfo.cpp \
	MipsISelDAGToDAG.cpp \
	MipsISelLowering.cpp \
	MipsJITInfo.cpp \
	MipsMachineFunction.cpp \
	MipsMCInstLower.cpp \
	MipsRegisterInfo.cpp \
	MipsSubtarget.cpp \
	MipsTargetMachine.cpp \
	MipsTargetObjectFile.cpp \
	MipsSelectionDAGInfo.cpp

TABLEGEN_SRC=		Mips.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/Mips
TABLEGEN_OUTPUT= \
	MipsGenRegisterInfo.inc|-gen-register-info \
	MipsGenInstrInfo.inc|-gen-instr-info \
	MipsGenAsmWriter.inc|-gen-asm-writer \
	MipsGenCallingConv.inc|-gen-callingconv \
	MipsGenCodeEmitter.inc|-gen-emitter \
	MipsGenDAGISel.inc|-gen-dag-isel \
	MipsGenDisassemblerTables.inc|-gen-disassembler \
	MipsGenEDInfo.inc|-gen-enhanced-disassembly-info \
	MipsGenMCCodeEmitter.inc|-gen-emitter^-mc-emitter \
	MipsGenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
