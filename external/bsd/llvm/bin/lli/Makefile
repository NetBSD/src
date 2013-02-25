#	$NetBSD: Makefile,v 1.6.8.2 2013/02/25 00:26:32 tls Exp $

PROG_CXX=	lli
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/lli

SRCS=	lli.cpp \
	RecordingMemoryManager.cpp \
	RemoteTarget.cpp

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
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	MC \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
