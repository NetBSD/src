#	$NetBSD: Makefile,v 1.3 2011/07/17 20:07:58 joerg Exp $

LIB=	LLVMMipsCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips

SRCS+=	MipsAsmPrinter.cpp \
	MipsDelaySlotFiller.cpp \
	MipsEmitGPRestore.cpp \
	MipsExpandPseudo.cpp \
	MipsInstrInfo.cpp \
	MipsISelDAGToDAG.cpp \
	MipsISelLowering.cpp \
	MipsFrameLowering.cpp \
	MipsMCInstLower.cpp \
	MipsMCSymbolRefExpr.cpp \
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
	MipsGenDAGISel.inc|-gen-dag-isel \
	MipsGenCallingConv.inc|-gen-callingconv \
	MipsGenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
