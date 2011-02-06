#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:45 joerg Exp $

PROG_CXX=	llvm-bcanalyzer
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-bcanalyzer

SRCS=	llvm-bcanalyzer.cpp

LLVM_LIBS+= \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
