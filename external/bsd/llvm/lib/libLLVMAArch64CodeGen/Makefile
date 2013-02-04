#	$NetBSD: Makefile,v 1.1 2013/02/04 15:00:33 joerg Exp $

LIB=	LLVMAArch64CodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/AArch64

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/AArch64

SRCS+=	AArch64AsmPrinter.cpp \
	AArch64ConstantIslandPass.cpp \
	AArch64FrameLowering.cpp \
	AArch64ISelDAGToDAG.cpp \
	AArch64ISelLowering.cpp \
	AArch64InstrInfo.cpp \
	AArch64MachineFunctionInfo.cpp \
	AArch64MCInstLower.cpp \
	AArch64RegisterInfo.cpp \
	AArch64SelectionDAGInfo.cpp \
	AArch64Subtarget.cpp \
	AArch64TargetMachine.cpp \
	AArch64TargetObjectFile.cpp

TABLEGEN_SRC=		AArch64.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/AArch64
TABLEGEN_OUTPUT= \
	AArch64GenAsmMatcher.inc|-gen-asm-matcher \
	AArch64GenAsmWriter.inc|-gen-asm-writer \
	AArch64GenCallingConv.inc|-gen-callingconv \
	AArch64GenDisassemblerTables.inc|-gen-disassembler \
	AArch64GenInstrInfo.inc|-gen-instr-info \
	AArch64GenMCCodeEmitter.inc|-gen-emitter^-mc-emitter \
	AArch64GenMCPseudoLowering.inc|-gen-pseudo-lowering \
	AArch64GenRegisterInfo.inc|-gen-register-info \
	AArch64GenDAGISel.inc|-gen-dag-isel \
	AArch64GenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
