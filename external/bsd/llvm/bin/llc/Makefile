#	$NetBSD: Makefile,v 1.8 2011/07/13 23:48:29 joerg Exp $

PROG_CXX=	llc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llc

SRCS=	llc.cpp

LLVM_LIBS+= \
	AsmPrinter \
	AsmParser \
	BitReader \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmPrinter \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	X86CodeGen \
	X86TargetInfo \
	X86AsmParser \
	X86AsmPrinter \
	X86Utils \
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	MCParser \
	MC \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
