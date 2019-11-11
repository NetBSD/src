#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:00 joerg Exp $

PROG_CXX=	llvm-dwarfdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dwarfdump

SRCS=	Statistics.cpp \
	llvm-dwarfdump.cpp

LLVM_LIBS+= \
	AArch64TargetInfo \
	AArch64MCTargetDesc \
	AArch64Utils \
	AMDGPUTargetInfo \
	AMDGPUMCTargetDesc \
	AMDGPUUtils \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMUtils \
	MipsTargetInfo \
	MipsMCTargetDesc \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	SparcTargetInfo \
	SparcMCTargetDesc \
	X86TargetInfo \
	X86MCTargetDesc \
	X86Utils \
	DebugInfoDWARF \
	Object \
	BitReader \
	BitstreamReader \
	IR \
	BinaryFormat \
	MCDisassembler \
	MCParser \
	MC \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
