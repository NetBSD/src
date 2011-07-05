#	$NetBSD: Makefile,v 1.4 2011/07/05 14:15:56 joerg Exp $

PROG_CXX=	llc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llc

SRCS=	llc.cpp

LLVM_LIBS+= \
	AsmPrinter \
	AsmParser \
	BitReader \
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
