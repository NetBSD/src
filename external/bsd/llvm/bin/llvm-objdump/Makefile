#	$NetBSD: Makefile,v 1.8 2014/01/15 22:21:34 joerg Exp $

PROG_CXX=	llvm-objdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-objdump

SRCS=	llvm-objdump.cpp \
	COFFDump.cpp \
	ELFDump.cpp \
	MachODump.cpp

LLVM_LIBS+= \
	AArch64Disassembler \
	AArch64TargetInfo \
	AArch64MCTargetDesc \
	AArch64AsmParser \
	AArch64AsmPrinter \
	AArch64Utils \
	ARMDisassembler \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	MipsDisassembler \
	MipsTargetInfo \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	MipsAsmParser \
	PowerPCDisassembler \
	PowerPCAsmParser \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcDisassembler \
	SparcAsmParser \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmPrinter \
	X86Disassembler \
	X86MCTargetDesc \
	X86TargetInfo \
	X86AsmParser \
	X86AsmPrinter \
	X86Utils \
	DebugInfo \
	MCParser \
	MC \
	Object \
	IR \
	Support

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
