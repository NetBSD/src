#	$NetBSD: Makefile,v 1.1.4.3 2014/05/22 16:13:23 yamt Exp $

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

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
