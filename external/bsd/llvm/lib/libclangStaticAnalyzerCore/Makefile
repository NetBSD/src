#	$NetBSD: Makefile,v 1.11 2012/08/09 14:37:07 joerg Exp $

LIB=	clangStaticAnalyzerCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Core

SRCS+=	APSIntType.cpp \
	AnalysisManager.cpp \
	BasicConstraintManager.cpp \
	BasicValueFactory.cpp \
	BlockCounter.cpp \
	BugReporter.cpp \
	BugReporterVisitors.cpp \
	CallEvent.cpp \
	Checker.cpp \
	CheckerContext.cpp \
	CheckerHelpers.cpp \
	CheckerManager.cpp \
	CheckerRegistry.cpp \
	CoreEngine.cpp \
	Environment.cpp \
	ExplodedGraph.cpp \
	ExprEngine.cpp \
	ExprEngineC.cpp \
	ExprEngineCXX.cpp \
	ExprEngineCallAndReturn.cpp \
	ExprEngineObjC.cpp \
	FunctionSummary.cpp \
	HTMLDiagnostics.cpp \
	MemRegion.cpp \
	PathDiagnostic.cpp \
	PlistDiagnostics.cpp \
	ProgramState.cpp \
	RangeConstraintManager.cpp \
	RegionStore.cpp \
	SimpleConstraintManager.cpp \
	SimpleSValBuilder.cpp \
	Store.cpp \
	SValBuilder.cpp \
	SVals.cpp \
	SubEngine.cpp \
	SymbolManager.cpp \
	TextPathDiagnostics.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
