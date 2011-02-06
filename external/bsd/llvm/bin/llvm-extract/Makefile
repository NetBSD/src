#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:46 joerg Exp $

PROG_CXX=	llvm-extract
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-extract

SRCS=	llvm-extract.cpp

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	BitWriter \
	Target \
	ipo \
	TransformsUtils \
	Analysis \
	ipa \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
