#	$NetBSD: Makefile,v 1.1.4.2 2013/01/16 05:27:58 yamt Exp $

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
