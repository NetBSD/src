#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:13:22 yamt Exp $

PROG_CXX=	llvm-bcanalyzer
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-bcanalyzer

SRCS=	llvm-bcanalyzer.cpp

LLVM_LIBS+= \
	BitReader \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
