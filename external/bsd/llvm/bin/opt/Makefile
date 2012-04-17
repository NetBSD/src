#	$NetBSD: Makefile,v 1.2.4.1 2012/04/17 00:03:29 yamt Exp $

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
