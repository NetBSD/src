#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:45 joerg Exp $

PROG_CXX=	llvm-diff
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-diff

SRCS=	llvm-diff.cpp \
	DifferenceEngine.cpp

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
