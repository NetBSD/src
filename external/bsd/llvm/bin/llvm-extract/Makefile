#	$NetBSD: Makefile,v 1.1.12.1 2012/11/20 02:58:00 tls Exp $

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
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
