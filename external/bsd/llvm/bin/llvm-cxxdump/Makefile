#	$NetBSD: Makefile,v 1.3 2017/08/01 20:07:38 joerg Exp $

PROG_CXX=	llvm-cxxdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cxxdump

SRCS=	llvm-cxxdump.cpp \
	Error.cpp

LLVM_LIBS+= \
	ARMTargetInfo \
	AArch64TargetInfo \
	MipsTargetInfo \
	PowerPCTargetInfo \
	SparcTargetInfo \
	X86TargetInfo \
	Object \
	BitReader \
	MCParser \
	MC \
	IR \
	BinaryFormat \
	Support \
	Demangle

#LDADD+=	-lz
#DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
