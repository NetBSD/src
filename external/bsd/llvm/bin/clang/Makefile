#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:44 joerg Exp $

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
	clangStaticAnalyzerCheckers \
	clangStaticAnalyzer \
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
	X86CodeGen \
	X86TargetInfo \
	X86AsmParser \
	X86Disassembler \
	X86AsmPrinter \
	SelectionDAG \
	AsmPrinter \
	CodeGen \
	Target \
	InstCombine \
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
