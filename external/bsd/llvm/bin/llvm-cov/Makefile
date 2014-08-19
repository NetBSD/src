#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:47:18 tls Exp $

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
