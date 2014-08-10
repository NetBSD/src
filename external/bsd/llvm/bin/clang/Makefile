#	$NetBSD: Makefile,v 1.32 2014/08/10 17:34:19 joerg Exp $

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
	clangEdit \
	clangAnalysis \
	clangFrontendRewrite \
	clangRewrite \
	clangAST \
	clangLex \
	clangBasic

LLVM_LIBS+= \
	ObjCARC \
	ipo \
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
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmPrinter \
	MipsAsmParser \
	MipsMCTargetDesc \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmParser \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmParser \
	SparcAsmPrinter \
	X86CodeGen \
	X86MCTargetDesc \
	X86TargetInfo \
	X86Utils \
	X86AsmParser \
	X86AsmPrinter \
	SelectionDAG \
	AsmPrinter \
	CodeGen \
	ProfileData \
	Vectorize \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	Analysis \
	MCDisassembler \
	MCParser \
	MC \
	Object \
	Linker \
	TransformsUtils \
	ipa \
	IR \
	Option \
	Support

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
