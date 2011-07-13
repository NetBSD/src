#	$NetBSD: Makefile,v 1.12 2011/07/13 23:48:29 joerg Exp $

PROG_CXX=	clang
NOMAN=		yes

.if !defined(HOSTPROG)
SYMLINKS+=	clang ${BINDIR}/clang++
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
	MipsAsmPrinter \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	X86CodeGen \
	X86MCTargetDesc \
	X86TargetInfo \
	X86Utils \
	X86AsmParser \
	X86AsmPrinter \
	SelectionDAG \
	AsmPrinter \
	CodeGen \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	Analysis \
	MCDisassembler \
	MCParser \
	MC \
	ipo \
	TransformsUtils \
	ipa \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
