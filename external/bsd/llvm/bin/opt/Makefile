#	$NetBSD: Makefile,v 1.19 2018/07/17 18:55:26 joerg Exp $

PROG_CXX=	opt
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/opt

SRCS=	AnalysisWrappers.cpp \
	BreakpointPrinter.cpp \
	Debugify.cpp \
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
	Linker \
	Object \
	IRReader \
	AsmParser \
	BitReader \
	BitWriter \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64Utils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMAsmPrinter \
	ARMUtils \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCAsmParser \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcAsmParser \
	SparcMCTargetDesc \
	SparcAsmPrinter \
	X86CodeGen \
	X86AsmParser \
	X86MCTargetDesc \
	X86TargetInfo \
	X86Utils \
	X86AsmPrinter \
	GlobalISel \
	TransformsCoroutines \
	MCDisassembler \
	AsmPrinter \
	DebugInfoCodeView \
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
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
