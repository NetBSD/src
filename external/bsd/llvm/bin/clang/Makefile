#	$NetBSD: Makefile,v 1.18 2012/02/28 17:09:29 joerg Exp $

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
	cc1as_main.cpp

CLANG_LIBS+= \
	clangFrontendTool \
	clangFrontend \
	clangARCMigrate \
	clangStaticAnalyzerFrontend \
	clangStaticAnalyzerCheckers \
	clangStaticAnalyzerCore \
	clangDriver \
	clangSerialization \
	clangCodeGen \
	clangParse \
	clangSema \
	clangAnalysis \
	clangIndex \
	clangRewrite \
	clangAST \
	clangLex \
	clangBasic

LLVM_LIBS+= \
	ipo \
	AsmParser \
	BitReader \
	BitWriter \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	MipsCodeGen \
	MipsTargetInfo \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	MipsAsmParser \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	X86CodeGen \
	X86MCTargetDesc \
	X86TargetInfo \
	X86Utils \
	X86AsmParser \
	X86AsmPrinter \
	SelectionDAG \
	AsmPrinter \
	CodeGen \
	Vectorize \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	Analysis \
	MCDisassembler \
	MCParser \
	MC \
	Linker \
	TransformsUtils \
	ipa \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
