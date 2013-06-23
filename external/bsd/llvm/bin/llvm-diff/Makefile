#	$NetBSD: Makefile,v 1.2.10.2 2013/06/23 06:28:21 tls Exp $

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
