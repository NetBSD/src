#	$NetBSD: Makefile,v 1.7 2017/08/01 20:07:38 joerg Exp $

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
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
