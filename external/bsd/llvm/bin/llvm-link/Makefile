#	$NetBSD: Makefile,v 1.2.8.1 2013/02/25 00:26:33 tls Exp $

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
