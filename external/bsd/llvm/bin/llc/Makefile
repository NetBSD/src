#	$NetBSD: Makefile,v 1.22 2016/02/27 21:47:30 joerg Exp $

PROG_CXX=	llc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llc

SRCS=	llc.cpp

LLVM_LIBS+= \
	MIRParser \
	AsmPrinter \
	IRReader \
	AsmParser \
	BitReader \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64Utils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsAsmPrinter \
	MipsMCTargetDesc \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmParser \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmParser \
	SparcAsmPrinter \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmParser \
	X86AsmPrinter \
	X86Utils \
	MCDisassembler \
	Object \
	SelectionDAG \
	CodeGen \
	Instrumentation \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	MCParser \
	MC \
	IR \
	Support

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
