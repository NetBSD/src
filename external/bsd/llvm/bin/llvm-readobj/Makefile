#	$NetBSD: Makefile,v 1.9 2016/02/27 21:47:31 joerg Exp $

PROG_CXX=	llvm-readobj
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-readobj

SRCS=	llvm-readobj.cpp \
	ARMAttributeParser.cpp \
	ARMWinEHPrinter.cpp \
	COFFDumper.cpp \
	COFFImportDumper.cpp \
	ELFDumper.cpp \
	Error.cpp \
	MachODumper.cpp \
	ObjDumper.cpp \
	StreamWriter.cpp \
	Win64EHDumper.cpp

LLVM_LIBS+= \
	AArch64TargetInfo \
	ARMTargetInfo \
	MipsTargetInfo \
	PowerPCTargetInfo \
	SparcTargetInfo \
	X86TargetInfo \
	Object \
	MC \
	MCParser \
	BitReader \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
