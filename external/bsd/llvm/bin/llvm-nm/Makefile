#	$NetBSD: Makefile,v 1.8 2018/07/17 18:55:26 joerg Exp $

PROG_CXX=	llvm-nm
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-nm

SRCS=	llvm-nm.cpp

LLVM_LIBS+= \
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
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
