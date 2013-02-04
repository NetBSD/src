#	$NetBSD: Makefile,v 1.2 2013/02/04 15:00:32 joerg Exp $

PROG_CXX=	llvm-symbolizer
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-symbolizer

SRCS=	LLVMSymbolize.cpp \
	llvm-symbolizer.cpp

LLVM_LIBS+= \
	DebugInfo \
	Object \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
