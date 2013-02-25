#	$NetBSD: Makefile,v 1.1.12.2 2013/02/25 00:26:33 tls Exp $

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
