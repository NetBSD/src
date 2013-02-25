#	$NetBSD: Makefile,v 1.1.12.1 2013/02/25 00:26:33 tls Exp $

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
