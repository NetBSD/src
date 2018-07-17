#	$NetBSD: Makefile,v 1.4 2018/07/17 18:55:25 joerg Exp $

PROG_CXX=	llvm-dwp
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dwp

SRCS=	DWPError.cpp \
	llvm-dwp.cpp

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
	ARMUtils \
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
	GlobalISel \
	AsmPrinter \
	DebugInfoCodeView \
	DebugInfoMSF \
	Object \
	MCParser \
	SelectionDAG \
	CodeGen \
	ProfileData \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	MC \
	TransformsUtils \
	Analysis \
	IR \
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
