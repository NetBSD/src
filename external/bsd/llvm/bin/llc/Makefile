#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:44 joerg Exp $

PROG_CXX=	llc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llc

SRCS=	llc.cpp

LLVM_LIBS+= \
	AsmPrinter \
	AsmParser \
	BitReader \
	X86CodeGen \
	X86TargetInfo \
	X86AsmParser \
	X86AsmPrinter \
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	Analysis \
	MCParser \
	MC \
	TransformsUtils \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
