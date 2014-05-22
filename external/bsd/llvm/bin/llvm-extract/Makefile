#	$NetBSD: Makefile,v 1.1.6.2 2014/05/22 16:13:22 yamt Exp $

PROG_CXX=	llvm-extract
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-extract

SRCS=	llvm-extract.cpp

LLVM_LIBS+= \
	IRReader \
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
