#	$NetBSD: Makefile,v 1.3.2.3 2013/06/23 06:28:22 tls Exp $

PROG_CXX=	opt
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/opt

SRCS=	AnalysisWrappers.cpp \
	GraphPrinters.cpp \
	PrintSCC.cpp \
	opt.cpp

LLVM_LIBS+= \
	ObjCARC \
	ipo \
	IRReader \
	AsmParser \
	BitReader \
	BitWriter \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64Utils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmPrinter \
	MipsCodeGen \
	MipsTargetInfo \
	MipsMCTargetDesc \
	MipsAsmPrinter \
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
	X86AsmPrinter \
	Object \
	SelectionDAG \
	CodeGen \
	Vectorize \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	Analysis \
	MC \
	TransformsUtils \
	ipa \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
