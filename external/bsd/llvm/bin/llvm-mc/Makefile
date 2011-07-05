#	$NetBSD: Makefile,v 1.5 2011/07/05 17:22:21 joerg Exp $

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
	ARMAsmPrinter \
	ARMDisassembler \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	X86CodeGen \
	X86TargetInfo \
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
