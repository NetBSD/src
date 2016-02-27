#	$NetBSD: Makefile,v 1.5 2016/02/27 21:47:31 joerg Exp $

PROG_CXX=	llvm-cov
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cov

SRCS=	gcov.cpp \
	llvm-cov.cpp \
	CodeCoverage.cpp \
	CoverageFilters.cpp \
	CoverageReport.cpp \
	CoverageSummaryInfo.cpp \
	SourceCoverageView.cpp \
	TestingSupport.cpp

LLVM_LIBS+= \
	ProfileData \
	Object \
	MCParser \
	MC \
	BitReader \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
