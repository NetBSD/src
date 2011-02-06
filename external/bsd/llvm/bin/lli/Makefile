#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:44 joerg Exp $

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
