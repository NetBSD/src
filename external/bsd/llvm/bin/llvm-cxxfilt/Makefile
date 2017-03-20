#	$NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:52:28 pgoyette Exp $

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
