#	$NetBSD: Makefile,v 1.11 2018/07/17 18:55:25 joerg Exp $

PROG_CXX=	llvm-ar
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-ar

SRCS=	llvm-ar.cpp

LLVM_LIBS+= \
	ToolDriversDlltool \
	ToolDrivers \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64TargetInfo \
	AArch64Utils \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMAsmPrinter \
	ARMTargetInfo \
	ARMUtils \
	MipsAsmParser \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	MipsTargetInfo \
	PowerPCAsmParser \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	PowerPCTargetInfo \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmParser \
	SparcAsmPrinter \
	X86AsmParser \
	X86CodeGen \
	X86MCTargetDesc \
	X86AsmPrinter \
	X86Utils \
	X86TargetInfo \
	CodeGen \
	MCDisassembler \
	Object \
	MCParser \
	MC \
	BitReader \
	IR \
	BinaryFormat \
	Option \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
