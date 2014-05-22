#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 16:13:22 yamt Exp $

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
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
