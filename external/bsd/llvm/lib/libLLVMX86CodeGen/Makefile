#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:03 joerg Exp $

LIB=	LLVMX86CodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/X86

SRCS+=	SSEDomainFix.cpp \
	X86AsmBackend.cpp \
	X86AsmPrinter.cpp \
	X86COFFMachineModuleInfo.cpp \
	X86CodeEmitter.cpp \
	X86ELFWriterInfo.cpp \
	X86FastISel.cpp \
	X86FloatingPoint.cpp \
	X86FrameLowering.cpp \
	X86ISelDAGToDAG.cpp \
	X86ISelLowering.cpp \
	X86InstrInfo.cpp \
	X86JITInfo.cpp \
	X86MachObjectWriter.cpp \
	X86MCAsmInfo.cpp \
	X86MCCodeEmitter.cpp \
	X86MCInstLower.cpp \
	X86RegisterInfo.cpp \
	X86SelectionDAGInfo.cpp \
	X86Subtarget.cpp \
	X86TargetMachine.cpp \
	X86TargetObjectFile.cpp

TABLEGEN_SRC=		X86.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/X86
TABLEGEN_OUTPUT= \
	X86GenRegisterInfo.h.inc|-gen-register-desc-header \
	X86GenRegisterNames.inc|-gen-register-enums \
	X86GenRegisterInfo.inc|-gen-register-desc \
	X86GenDisassemblerTables.inc|-gen-disassembler \
	X86GenInstrNames.inc|-gen-instr-enums \
	X86GenInstrInfo.inc|-gen-instr-desc \
	X86GenAsmWriter.inc|-gen-asm-writer \
	X86GenAsmWriter1.inc|-gen-asm-writer^-asmwriternum=1 \
	X86GenAsmMatcher.inc|-gen-asm-matcher \
	X86GenDAGISel.inc|-gen-dag-isel \
	X86GenFastISel.inc|-gen-fast-isel \
	X86GenCallingConv.inc|-gen-callingconv \
	X86GenSubtarget.inc|-gen-subtarget \
	X86GenEDInfo.inc|-gen-enhanced-disassembly-info

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
