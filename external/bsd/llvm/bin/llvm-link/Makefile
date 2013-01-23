#	$NetBSD: Makefile,v 1.3 2013/01/23 15:02:55 joerg Exp $

PROG_CXX=	llvm-link
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-link

SRCS=	llvm-link.cpp

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	BitWriter \
	Linker \
	TransformsUtils \
	Analysis \
	Target \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
