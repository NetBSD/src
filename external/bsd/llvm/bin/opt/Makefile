#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:48 joerg Exp $

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
