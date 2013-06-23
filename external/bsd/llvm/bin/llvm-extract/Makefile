#	$NetBSD: Makefile,v 1.1.12.3 2013/06/23 06:28:21 tls Exp $

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
