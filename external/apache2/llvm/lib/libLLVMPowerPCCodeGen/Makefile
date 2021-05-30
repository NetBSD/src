#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:54 joerg Exp $

LIB=	LLVMPowerPCCodeGen

.include <bsd.init.mk>

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/PowerPC

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC

SRCS+=	PPCAsmPrinter.cpp \
	PPCBoolRetToInt.cpp \
	PPCBranchCoalescing.cpp \
	PPCBranchSelector.cpp \
	PPCCallingConv.cpp \
	PPCCCState.cpp \
	PPCCTRLoops.cpp \
	PPCEarlyReturn.cpp \
	PPCExpandISEL.cpp \
	PPCFastISel.cpp \
	PPCFrameLowering.cpp \
	PPCHazardRecognizers.cpp \
	PPCInstrInfo.cpp \
	PPCISelDAGToDAG.cpp \
	PPCISelLowering.cpp \
	PPCLoopInstrFormPrep.cpp \
	PPCLowerMASSVEntries.cpp \
	PPCMachineFunctionInfo.cpp \
	PPCMachineScheduler.cpp \
	PPCMacroFusion.cpp \
	PPCMCInstLower.cpp \
	PPCMIPeephole.cpp \
	PPCPreEmitPeephole.cpp \
	PPCReduceCRLogicals.cpp \
	PPCRegisterInfo.cpp \
	PPCSubtarget.cpp \
	PPCTargetMachine.cpp \
	PPCTargetObjectFile.cpp \
	PPCTargetTransformInfo.cpp \
	PPCTLSDynamicCall.cpp \
	PPCTOCRegDeps.cpp \
	PPCVSXCopy.cpp \
	PPCVSXFMAMutate.cpp \
	PPCVSXSwapRemoval.cpp

.PATH: ${LLVM_SRCDIR}/lib/Target/PowerPC/GISel

SRCS+=	PPCCallLowering.cpp \
	PPCInstructionSelector.cpp \
	PPCLegalizerInfo.cpp \
	PPCRegisterBankInfo.cpp

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
	PPCGenGlobalISel.inc|-gen-global-isel \
	PPCGenInstrInfo.inc|-gen-instr-info \
	PPCGenMCCodeEmitter.inc|-gen-emitter \
	PPCGenRegisterBank.inc|-gen-register-bank \
	PPCGenRegisterInfo.inc|-gen-register-info \
	PPCGenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
