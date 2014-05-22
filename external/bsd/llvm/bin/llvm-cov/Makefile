#	$NetBSD: Makefile,v 1.1.2.1 2014/05/22 16:13:22 yamt Exp $

PROG_CXX=	llvm-cov
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cov

SRCS=	llvm-cov.cpp

LLVM_LIBS+= \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
