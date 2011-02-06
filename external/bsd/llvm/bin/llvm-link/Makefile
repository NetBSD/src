#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:46 joerg Exp $

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
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
