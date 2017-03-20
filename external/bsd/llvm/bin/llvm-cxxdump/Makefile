#	$NetBSD: Makefile,v 1.1.2.1 2017/03/20 06:52:27 pgoyette Exp $

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
	Support \
	Demangle

#LDADD+=	-lz
#DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
