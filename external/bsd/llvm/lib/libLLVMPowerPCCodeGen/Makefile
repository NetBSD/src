#	$NetBSD: Makefile,v 1.13 2017/08/01 20:07:41 joerg Exp $

LIB=	LLVMPowerPCCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC

SRCS+=	PPCAsmPrinter.cpp \
	PPCBoolRetToInt.cpp \
	PPCBranchSelector.cpp \
	PPCCCState.cpp \
	PPCCTRLoops.cpp \
	PPCEarlyReturn.cpp \
	PPCExpandISEL.cpp \
	PPCHazardRecognizers.cpp \
	PPCInstrInfo.cpp \
	PPCISelDAGToDAG.cpp \
	PPCISelLowering.cpp \
	PPCFastISel.cpp \
	PPCFrameLowering.cpp \
	PPCLoopPreIncPrep.cpp \
	PPCMachineFunctionInfo.cpp \
	PPCMCInstLower.cpp \
	PPCMIPeephole.cpp \
	PPCQPXLoadSplat.cpp \
	PPCRegisterInfo.cpp \
	PPCSubtarget.cpp \
	PPCTLSDynamicCall.cpp \
	PPCTOCRegDeps.cpp \
	PPCTargetMachine.cpp \
	PPCTargetObjectFile.cpp \
	PPCTargetTransformInfo.cpp \
	PPCVSXCopy.cpp \
	PPCVSXFMAMutate.cpp \
	PPCVSXSwapRemoval.cpp

TABLEGEN_SRC=		PPC.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/PowerPC
TABLEGEN_OUTPUT= \
	PPCGenAsmMatcher.inc|-gen-asm-matcher \
	PPCGenAsmWriter.inc|-gen-asm-writer \
	PPCGenCallingConv.inc|-gen-callingconv \
	PPCGenCodeEmitter.inc|-gen-emitter \
	PPCGenDAGISel.inc|-gen-dag-isel \
	PPCGenDisassemblerTables.inc|-gen-disassembler \
	PPCGenFastISel.inc|-gen-fast-isel \
	PPCGenInstrInfo.inc|-gen-instr-info \
	PPCGenMCCodeEmitter.inc|-gen-emitter \
	PPCGenRegisterInfo.inc|-gen-register-info \
	PPCGenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
