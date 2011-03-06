#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:39 bouyer Exp $

PROG_CXX=	lli
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/lli

SRCS=	lli.cpp

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	JIT \
	MCJIT \
	Interpreter \
	ExecutionEngine \
	X86CodeGen \
	X86TargetInfo \
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	Analysis \
	MC \
	TransformsUtils \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
