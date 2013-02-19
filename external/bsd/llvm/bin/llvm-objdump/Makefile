#	$NetBSD: Makefile,v 1.3 2013/02/19 12:35:42 joerg Exp $

PROG_CXX=	llvm-objdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-objdump

SRCS=	llvm-objdump.cpp \
	COFFDump.cpp \
	ELFDump.cpp \
	MachODump.cpp \
	MCFunction.cpp \

LLVM_LIBS+= \
	Object \
	AArch64Disassembler \
	AArch64TargetInfo \
	AArch64MCTargetDesc \
	AArch64AsmParser \
	AArch64AsmPrinter \
	AArch64Utils \
	ARMDisassembler \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	MipsDisassembler \
	MipsTargetInfo \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	MipsAsmParser \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcTargetInfo \
	SparcMCTargetDesc \
	X86Disassembler \
	X86MCTargetDesc \
	X86TargetInfo \
	X86AsmParser \
	X86AsmPrinter \
	X86Utils \
	DebugInfo \
	MCParser \
	MC \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
