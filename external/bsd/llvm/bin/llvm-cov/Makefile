#	$NetBSD: Makefile,v 1.3 2014/08/10 17:34:20 joerg Exp $

PROG_CXX=	llvm-cov
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cov

SRCS=	gcov.cpp \
	llvm-cov.cpp

LLVM_LIBS+= \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
