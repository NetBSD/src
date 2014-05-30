#	$NetBSD: Makefile,v 1.7 2014/05/30 18:28:13 joerg Exp $

PROG_CXX=	llvm-readobj
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-readobj

SRCS=	llvm-readobj.cpp \
	ARMAttributeParser.cpp \
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
	MC \
	BitReader \
	Object \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
