#	$NetBSD: Makefile,v 1.1 2016/02/27 21:47:31 joerg Exp $

PROG_CXX=	llvm-dwp
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dwp

SRCS=	llvm-dwp.cpp

LLVM_LIBS+= \
	DebugInfoDWARF \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64Utils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmPrinter \
	MipsCodeGen \
	MipsTargetInfo \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmPrinter \
	X86CodeGen \
	X86MCTargetDesc \
	X86TargetInfo \
	X86Utils \
	X86AsmPrinter \
	MCDisassembler \
	AsmPrinter \
	Object \
	MCParser \
	SelectionDAG \
	CodeGen \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	MC \
	TransformsUtils \
	Analysis \
	IR \
	Support

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
