#	$NetBSD: Makefile,v 1.1 2019/11/11 22:44:59 joerg Exp $

PROG_CXX=	llvm-cxxfilt
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cxxfilt

SRCS=	llvm-cxxfilt.cpp

LLVM_LIBS+= \
	Demangle \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
