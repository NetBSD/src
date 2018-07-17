#	$NetBSD: Makefile,v 1.17 2018/07/17 18:55:25 joerg Exp $

PROG_CXX=	lli
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/lli

SRCS=	lli.cpp

LLVM_LIBS+= \
	Orc \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmPrinter \
	X86Utils \
	AsmPrinter \
	IRReader \
	AsmParser \
	Object \
	MCParser \
	BitWriter \
	BitReader \
	MCJIT \
	RuntimeDyld \
	Instrumentation \
	Interpreter \
	ExecutionEngine \
	SelectionDAG \
	CodeGen \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	MC \
	IR \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
