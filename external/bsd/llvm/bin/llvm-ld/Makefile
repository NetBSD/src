#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:41 bouyer Exp $

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
	ScalarOpts \
	Analysis \
	Target \
	ipo \
	TransformsUtils \
	ipa \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
