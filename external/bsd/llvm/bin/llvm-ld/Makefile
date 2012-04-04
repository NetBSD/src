#	$NetBSD: Makefile,v 1.3 2012/04/04 10:42:01 joerg Exp $

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
