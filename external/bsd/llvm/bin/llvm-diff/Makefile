#	$NetBSD: Makefile,v 1.4.12.1 2017/03/20 06:52:28 pgoyette Exp $

PROG_CXX=	llvm-diff
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-diff

SRCS=	llvm-diff.cpp \
	DiffConsumer.cpp \
	DiffLog.cpp \
	DifferenceEngine.cpp

LLVM_LIBS+= \
	IRReader \
	AsmParser \
	BitReader \
	IR \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
