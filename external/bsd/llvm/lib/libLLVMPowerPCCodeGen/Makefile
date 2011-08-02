#	$NetBSD: Makefile,v 1.3 2011/08/02 17:12:39 joerg Exp $

LIB=	LLVMPowerPCCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC

SRCS+=	PPCAsmPrinter.cpp \
	PPCBranchSelector.cpp \
	PPCCodeEmitter.cpp \
	PPCHazardRecognizers.cpp \
	PPCInstrInfo.cpp \
	PPCISelDAGToDAG.cpp \
	PPCISelLowering.cpp \
	PPCFrameLowering.cpp \
	PPCJITInfo.cpp \
	PPCMCInstLower.cpp \
	PPCRegisterInfo.cpp \
	PPCSubtarget.cpp \
	PPCTargetMachine.cpp \
	PPCSelectionDAGInfo.cpp

TABLEGEN_SRC=		PPC.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/PowerPC
TABLEGEN_OUTPUT= \
	PPCGenAsmWriter.inc|-gen-asm-writer \
	PPCGenCodeEmitter.inc|-gen-emitter \
	PPCGenMCCodeEmitter.inc|-gen-emitter^-mc-emitter \
	PPCGenRegisterInfo.inc|-gen-register-info \
	PPCGenInstrInfo.inc|-gen-instr-info \
	PPCGenDAGISel.inc|-gen-dag-isel \
	PPCGenCallingConv.inc|-gen-callingconv \
	PPCGenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
