#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:47 joerg Exp $

PROG_CXX=	llvm-mc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-mc

SRCS=	llvm-mc.cpp \
	Disassembler.cpp

LLVM_LIBS+= \
	X86CodeGen \
	X86TargetInfo \
	X86AsmParser \
	X86Disassembler \
	X86AsmPrinter \
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	Analysis \
	AsmPrinter \
	MCDisassembler \
	MCParser \
	MC \
	TransformsUtils \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
