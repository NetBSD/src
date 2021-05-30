#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:46 joerg Exp $

PROG_CXX=	llc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llc

SRCS=	llc.cpp

LLVM_LIBS+= \
	Vectorize \
	MIRParser \
	AsmPrinter \
	DebugInfoCodeView \
	DebugInfoDWARF \
	DebugInfoMSF \
	IRReader \
	AsmParser \
	BitReader \
	BitstreamReader \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64Utils \
	AMDGPUCodeGen \
	AMDGPUTargetInfo \
	AMDGPUMCTargetDesc \
	AMDGPUAsmParser \
	AMDGPUUtils \
	MIRParser \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMUtils \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsMCTargetDesc \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmParser \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmParser \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmParser \
	Passes \
	TransformsCFGuard \
	ipo \
	FrontendOpenMP \
	GlobalISel \
	MCDisassembler \
	Object \
	SelectionDAG \
	CodeGen \
	ProfileData \
	Instrumentation \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	MCParser \
	MC \
	IR \
	Remarks \
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
