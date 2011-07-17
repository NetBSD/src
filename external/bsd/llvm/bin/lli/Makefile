#	$NetBSD: Makefile,v 1.5 2011/07/17 20:07:57 joerg Exp $

PROG_CXX=	lli
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/lli

SRCS=	lli.cpp

LLVM_LIBS+= \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmPrinter \
	X86Utils \
	AsmPrinter \
	AsmParser \
	MCParser \
	BitReader \
	JIT \
	MCJIT \
	RuntimeDyld \
	Interpreter \
	ExecutionEngine \
	Object \
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
