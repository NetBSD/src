#	$NetBSD: Makefile,v 1.3 2017/01/11 12:02:25 joerg Exp $

PROG_CXX=	llvm-bcanalyzer
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-bcanalyzer

SRCS=	llvm-bcanalyzer.cpp

LLVM_LIBS+= \
	BitReader \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
