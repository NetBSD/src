#	$NetBSD: Makefile,v 1.11.2.1 2012/11/20 02:58:00 tls Exp $

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
