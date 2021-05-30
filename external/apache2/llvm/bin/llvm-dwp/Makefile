#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:47 joerg Exp $

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
	AArch64Utils \
	AMDGPUCodeGen \
	AMDGPUTargetInfo \
	AMDGPUMCTargetDesc \
	AMDGPUUtils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMUtils \
	MipsCodeGen \
	MipsTargetInfo \
	MipsMCTargetDesc \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	X86CodeGen \
	X86MCTargetDesc \
	X86TargetInfo \
	Passes \
	TransformsCFGuard \
	MIRParser \
	Vectorize \
	MCDisassembler \
	ipo \
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
	Remarks \
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
