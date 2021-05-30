#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:47 joerg Exp $

PROG_CXX=	llvm-dwarfdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dwarfdump

SRCS=	llvm-dwarfdump.cpp \
	SectionSizes.cpp \
	Statistics.cpp \

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
	DebugInfoDWARF \
	Object \
	TextAPI \
	BitReader \
	BitstreamReader \
	IR \
	MCDisassembler \
	MCParser \
	MC \
	Remarks \
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
