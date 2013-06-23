#	$NetBSD: Makefile,v 1.11.2.3 2013/06/23 06:28:21 tls Exp $

PROG_CXX=	llvm-mc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-mc

SRCS=	llvm-mc.cpp \
	Disassembler.cpp

LLVM_LIBS+= \
	MCDisassembler \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64Disassembler \
	AArch64Utils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMAsmPrinter \
	ARMDisassembler \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsDisassembler \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmParser \
	X86Disassembler \
	X86AsmPrinter \
	X86Utils \
	Object \
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	AsmPrinter \
	MCParser \
	MC \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
