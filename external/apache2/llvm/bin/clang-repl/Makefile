#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:46 joerg Exp $

PROG_CXX=	clang-repl
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/tools/clang-repl

SRCS=	ClangRepl.cpp

.include "${.PARSEDIR}/../../link.mk"

CLANG_LIBS+= \
	clangInterpreter \
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
	TransformsCoroutines \
	ProfileDataCoverage \
	LTO \
	ObjCARC \
	Passes \
	TransformsAggressiveInstCombine \
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
	Orc \
	ExecutionEngineJITLink \
	ExecutionEngineOrcTargetProcess \
	ExecutionEngineOrcShared \
	ExecutionEngine \
	RuntimeDyld \
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
	LineEditor \
	Option \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz -ledit
DPADD+=	${LIBZ} ${LIBEDIT}

.include <bsd.prog.mk>
