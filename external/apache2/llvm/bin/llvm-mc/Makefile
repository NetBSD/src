#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:58 cjep Exp $

PROG_CXX=	llvm-mc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-mc

SRCS=	llvm-mc.cpp \
	Disassembler.cpp

LLVM_LIBS+= \
	MCDisassembler \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64Disassembler \
	AArch64Utils \
	AMDGPUCodeGen \
	AMDGPUTargetInfo \
	AMDGPUMCTargetDesc \
	AMDGPUAsmParser \
	AMDGPUDisassembler \
	AMDGPUUtils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMDisassembler \
	ARMUtils \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsDisassembler \
	MipsMCTargetDesc \
	PowerPCCodeGen \
	PowerPCAsmParser \
	PowerPCDisassembler \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	SparcAsmParser \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcDisassembler \
	X86AsmParser \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86Disassembler \
	MCDisassembler \
	Object \
	SelectionDAG \
	CodeGen \
	Target \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	AsmPrinter \
	MCParser \
	MC \
	BinaryFormat \
	IR \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
