#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:57 cjep Exp $

PROG_CXX=	llvm-cov
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-cov

SRCS=	gcov.cpp \
	llvm-cov.cpp \
	CodeCoverage.cpp \
	CoverageExporterJson.cpp \
	CoverageExporterLcov.cpp \
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
	TextAPI \
	MCParser \
	MC \
	BitReader \
	BitstreamReader \
	IR \
	Remarks \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
