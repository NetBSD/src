#	$NetBSD: Makefile,v 1.6 2017/01/11 12:02:26 joerg Exp $

PROG_CXX=	llvm-cov
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cov

SRCS=	gcov.cpp \
	llvm-cov.cpp \
	CodeCoverage.cpp \
	CoverageExporterJson.cpp \
	CoverageFilters.cpp \
	CoverageReport.cpp \
	CoverageSummaryInfo.cpp \
	SourceCoverageView.cpp \
	SourceCoverageViewHTML.cpp \
	SourceCoverageViewText.cpp \
	TestingSupport.cpp

LLVM_LIBS+= \
	ProfileDataCoverage \
	ProfileData \
	Object \
	MCParser \
	MC \
	BitReader \
	IR \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
