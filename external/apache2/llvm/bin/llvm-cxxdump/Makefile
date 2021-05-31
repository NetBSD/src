#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:57 cjep Exp $

PROG_CXX=	llvm-cxxdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cxxdump

SRCS=	llvm-cxxdump.cpp \
	Error.cpp

LLVM_LIBS+= \
	ARMTargetInfo \
	AMDGPUTargetInfo \
	AArch64TargetInfo \
	MipsTargetInfo \
	PowerPCTargetInfo \
	SparcTargetInfo \
	X86TargetInfo \
	Object \
	TextAPI \
	BitReader \
	BitstreamReader \
	MCParser \
	MC \
	IR \
	Remarks \
	BinaryFormat \
	Support \
	Demangle

#LDADD+=	-lz
#DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
