#	$NetBSD: Makefile,v 1.1 2019/11/11 22:44:59 joerg Exp $

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
	X86AsmParser \
	X86Utils \
	AsmPrinter \
	GlobalISel \
	IRReader \
	AsmParser \
	DebugInfoCodeView \
	DebugInfoDWARF \
	Object \
	MCParser \
	BitWriter \
	BitReader \
	BitstreamReader \
	MCDisassembler \
	MCJIT \
	RuntimeDyld \
	Instrumentation \
	Interpreter \
	ExecutionEngine \
	SelectionDAG \
	CodeGen \
	ProfileData \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	MC \
	IR \
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
