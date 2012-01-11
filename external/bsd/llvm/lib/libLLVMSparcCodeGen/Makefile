#	$NetBSD: Makefile,v 1.3 2012/01/11 23:24:48 joerg Exp $

LIB=	LLVMSparcCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/Sparc

SRCS+=	DelaySlotFiller.cpp \
	FPMover.cpp \
	SparcAsmPrinter.cpp \
	SparcInstrInfo.cpp \
	SparcISelDAGToDAG.cpp \
	SparcISelLowering.cpp \
	SparcFrameLowering.cpp \
	SparcMachineFunctionInfo.cpp \
	SparcRegisterInfo.cpp \
	SparcSubtarget.cpp \
	SparcTargetMachine.cpp \
	SparcSelectionDAGInfo.cpp


TABLEGEN_SRC=		Sparc.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/Sparc
TABLEGEN_OUTPUT= \
	SparcGenRegisterInfo.inc|-gen-register-info \
	SparcGenInstrInfo.inc|-gen-instr-info \
	SparcGenAsmWriter.inc|-gen-asm-writer \
	SparcGenDAGISel.inc|-gen-dag-isel \
	SparcGenSubtargetInfo.inc|-gen-subtarget \
	SparcGenCallingConv.inc|-gen-callingconv \

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
