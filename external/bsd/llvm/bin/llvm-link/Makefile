#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:41 bouyer Exp $

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
