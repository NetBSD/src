#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:59 cjep Exp $

PROG_CXX=	opt
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/opt

SRCS=	AnalysisWrappers.cpp \
	BreakpointPrinter.cpp \
	GraphPrinters.cpp \
	NewPMDriver.cpp \
	PassPrinters.cpp \
	PrintSCC.cpp \
	opt.cpp

LLVM_LIBS+= \
	TransformsAggressiveInstCombine \
	Passes \
	ObjCARC \
	ipo \
	FrontendOpenMP \
	Linker \
	Object \
	IRReader \
	AsmParser \
	BitReader \
	BitstreamReader \
	BitWriter \
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
	ARMCodeGen \
	ARMTargetInfo \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMUtils \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsMCTargetDesc \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCAsmParser \
	PowerPCMCTargetDesc \
	SparcCodeGen \
	SparcTargetInfo \
	SparcAsmParser \
	SparcMCTargetDesc \
	X86CodeGen \
	X86AsmParser \
	X86MCTargetDesc \
	X86TargetInfo \
	TransformsCFGuard \
	MIRParser \
	GlobalISel \
	TransformsCoroutines \
	MCDisassembler \
	AsmPrinter \
	DebugInfoCodeView \
	DebugInfoDWARF \
	DebugInfoMSF \
	Object \
	MCParser \
	SelectionDAG \
	CodeGen \
	Vectorize \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	ProfileData \
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
