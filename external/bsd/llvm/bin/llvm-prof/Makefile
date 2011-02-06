#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:47 joerg Exp $

PROG_CXX=	llvm-prof
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-prof

SRCS=	llvm-prof.cpp

LLVM_LIBS+= \
	BitReader \
	Analysis \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
