#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:10 joerg Exp $

LIB=	clangStaticAnalyzer

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer

SRCS+=	AggExprVisitor.cpp \
	AnalysisManager.cpp \
	AnalyzerStatsChecker.cpp \
	BasicConstraintManager.cpp \
	BasicStore.cpp \
	BasicValueFactory.cpp \
	BugReporter.cpp \
	BugReporterVisitors.cpp \
	CFRefCount.cpp \
	Checker.cpp \
	CheckerHelpers.cpp \
	Environment.cpp \
	ExplodedGraph.cpp \
	FlatStore.cpp \
	BlockCounter.cpp \
	CXXExprEngine.cpp \
	CoreEngine.cpp \
	GRState.cpp \
	HTMLDiagnostics.cpp \
	ManagerRegistry.cpp \
	MemRegion.cpp \
	ObjCMessage.cpp \
	PathDiagnostic.cpp \
	PlistDiagnostics.cpp \
	RangeConstraintManager.cpp \
	RegionStore.cpp \
	SimpleConstraintManager.cpp \
	SimpleSValBuilder.cpp \
	Store.cpp \
	SValBuilder.cpp \
	SVals.cpp \
	SymbolManager.cpp \
	TextPathDiagnostics.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
