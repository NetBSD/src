#	$NetBSD: Makefile,v 1.9 2011/07/17 20:07:57 joerg Exp $

PROG_CXX=	llvm-mc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-mc

SRCS=	llvm-mc.cpp \
	Disassembler.cpp

LLVM_LIBS+= \
	MCDisassembler \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	ARMDisassembler \
	MipsCodeGen \
	MipsTargetInfo \
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
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	AsmPrinter \
	MCParser \
	MC \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
