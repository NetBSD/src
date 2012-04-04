#	$NetBSD: Makefile,v 1.3 2012/04/04 10:42:01 joerg Exp $

PROG_CXX=	opt
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/opt

SRCS=	AnalysisWrappers.cpp \
	GraphPrinters.cpp \
	PrintSCC.cpp \
	opt.cpp

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	BitWriter \
	InstCombine \
	Instrumentation \
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
