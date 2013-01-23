#	$NetBSD: Makefile,v 1.3 2013/01/23 15:02:55 joerg Exp $

PROG_CXX=	llvm-extract
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-extract

SRCS=	llvm-extract.cpp

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	BitWriter \
	ipo \
	TransformsUtils \
	Analysis \
	Target \
	ipa \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
