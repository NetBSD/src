#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:58 cjep Exp $

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
	Remarks \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
