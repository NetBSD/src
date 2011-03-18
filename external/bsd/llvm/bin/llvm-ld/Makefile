#	$NetBSD: Makefile,v 1.2 2011/03/18 23:32:02 joerg Exp $

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
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	ipa \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
