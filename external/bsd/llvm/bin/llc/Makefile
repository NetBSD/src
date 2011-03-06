#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:39 bouyer Exp $

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
