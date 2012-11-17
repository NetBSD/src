#	$NetBSD: Makefile,v 1.1.2.2 2012/11/17 04:57:27 joerg Exp $

PROG_CXX=	llvm-symbolizer
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-symbolizer

SRCS=	llvm-symbolizer.cpp

LLVM_LIBS+= \
	DebugInfo \
	Object \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
