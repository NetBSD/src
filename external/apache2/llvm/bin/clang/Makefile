#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:46 joerg Exp $

PROG_CXX=	clang
NOMAN=		yes

.if !defined(HOSTPROG)
SYMLINKS+=	clang ${BINDIR}/clang++
SYMLINKS+=	clang ${BINDIR}/clang-cpp
.endif

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/tools/driver

SRCS=	driver.cpp \
	cc1_main.cpp \
	cc1as_main.cpp \
	cc1gen_reproducer_main.cpp

CLANG_LIBS+= \
	clangFrontendTool \
	clangFrontend \
	clangARCMigrate \
	clangStaticAnalyzerFrontend \
	clangStaticAnalyzerCheckers \
	clangStaticAnalyzerCore \
	clangToolingCore \
	clangCrossTU \
	clangIndex \
	clangASTMatchers \
	clangCodeGen \
	clangFrontendRewrite \
	clangFrontend \
	clangSerialization \
	clangParse \
	clangSema \
	clangAnalysis \
	clangEdit \
	clangDriver \
	clangRewrite \
	clangAST \
	clangLex \
	clangBasic

LLVM_LIBS+= \
	FrontendOpenMP \
	TransformsAggressiveInstCombine \
	TransformsCoroutines \
	ProfileDataCoverage \
	LTO \
	ObjCARC \
	Passes \
	ipo \
	IRReader \
	AsmParser \
	BitstreamReader \
	BitReader \
	BitWriter

.if !defined(HOSTPROG)
LLVM_LIBS+= \
	AMDGPUCodeGen \
	AMDGPUTargetInfo \
	AMDGPUAsmParser \
	AMDGPUMCTargetDesc \
	AMDGPUUtils \
	MIRParser
.endif

LLVM_LIBS+= \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64Utils \
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
	X86MCTargetDesc \
	X86TargetInfo \
	X86AsmParser \
	GlobalISel \
	SelectionDAG \
	AsmPrinter \
	DebugInfoDWARF \
	DebugInfoCodeView \
	DebugInfoMSF \
	CodeGen \
	Vectorize \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	ProfileData \
	MCDisassembler \
	MCParser \
	MC \
	TransformsUtils \
	Analysis \
	Object \
	TextAPI \
	Linker \
	TransformsCFGuard \
	IR \
	Remarks \
	Option \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
