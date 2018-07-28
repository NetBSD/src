#	$NetBSD: Makefile,v 1.5.2.1 2018/07/28 04:33:05 pgoyette Exp $

PROG_CXX=	llvm-dwarfdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dwarfdump

SRCS=	Statistics.cpp \
	llvm-dwarfdump.cpp

LLVM_LIBS+= \
	AArch64TargetInfo \
	AArch64MCTargetDesc \
	AArch64AsmPrinter \
	AArch64Utils \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmPrinter \
	ARMUtils \
	MipsTargetInfo \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmPrinter \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmPrinter \
	X86Utils \
	DebugInfoDWARF \
	Object \
	BitReader \
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
