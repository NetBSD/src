#	$NetBSD: Makefile,v 1.2 2011/03/18 23:32:01 joerg Exp $

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
	X86Utils \
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	MC \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
