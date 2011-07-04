#	$NetBSD: Makefile,v 1.3 2011/07/04 22:37:59 joerg Exp $

PROG_CXX=	llvm-mc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-mc

SRCS=	llvm-mc.cpp \
	Disassembler.cpp

LLVM_LIBS+= \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCAsmPrinter \
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
	MCDisassembler \
	MCParser \
	MC \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
