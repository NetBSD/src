#	$NetBSD: Makefile,v 1.1.4.3 2014/08/19 23:47:18 tls Exp $

PROG_CXX=	llvm-readobj
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-readobj

SRCS=	llvm-readobj.cpp \
	ARMAttributeParser.cpp \
	ARMWinEHPrinter.cpp \
	COFFDumper.cpp \
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
