#	$NetBSD: Makefile,v 1.1 2019/11/11 22:44:59 joerg Exp $

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
	BitstreamReader \
	IR \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
