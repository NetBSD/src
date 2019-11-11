#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:00 joerg Exp $

PROG_CXX=	llvm-nm
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-nm

SRCS=	llvm-nm.cpp

LLVM_LIBS+= \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64TargetInfo \
	AArch64Utils \
	AMDGPUTargetInfo \
	AMDGPUMCTargetDesc \
	AMDGPUAsmParser \
	AMDGPUUtils \
	ARMAsmParser \
	ARMMCTargetDesc \
	ARMTargetInfo \
	ARMUtils \
	MipsAsmParser \
	MipsMCTargetDesc \
	MipsTargetInfo \
	PowerPCAsmParser \
	PowerPCMCTargetDesc \
	PowerPCTargetInfo \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmParser \
	X86AsmParser \
	X86CodeGen \
	X86MCTargetDesc \
	X86Utils \
	X86TargetInfo \
	CodeGen \
	MCDisassembler \
	Object \
	MCParser \
	MC \
	BitReader \
	BitstreamReader \
	IR \
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
