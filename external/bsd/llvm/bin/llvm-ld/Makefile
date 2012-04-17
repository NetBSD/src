#	$NetBSD: Makefile,v 1.2.4.1 2012/04/17 00:03:28 yamt Exp $

PROG_CXX=	llvm-ld
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-ld

SRCS=	Optimize.cpp \
	llvm-ld.cpp

LLVM_LIBS+= \
	Linker \
	Archive \
	AsmParser \
	BitReader \
	BitWriter \
	InstCombine \
	ipo \
	Vectorize \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	ipa \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
