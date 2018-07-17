#	$NetBSD: Makefile,v 1.17 2018/07/17 18:55:30 joerg Exp $

LIB=	clangStaticAnalyzerCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Core

SRCS+=	APSIntType.cpp \
	AnalysisManager.cpp \
	AnalyzerOptions.cpp \
	BasicValueFactory.cpp \
	BlockCounter.cpp \
	BugReporter.cpp \
	BugReporterVisitors.cpp \
	CallEvent.cpp \
	CheckerContext.cpp \
	Checker.cpp \
	CheckerHelpers.cpp \
	CheckerManager.cpp \
	CheckerRegistry.cpp \
	CommonBugCategories.cpp \
	ConstraintManager.cpp \
	CoreEngine.cpp \
	DynamicTypeMap.cpp \
	Environment.cpp \
	ExplodedGraph.cpp \
	ExprEngineCallAndReturn.cpp \
	ExprEngineC.cpp \
	ExprEngine.cpp \
	ExprEngineCXX.cpp \
	ExprEngineObjC.cpp \
	FunctionSummary.cpp \
	HTMLDiagnostics.cpp \
	IssueHash.cpp \
	LoopUnrolling.cpp \
	LoopWidening.cpp \
	MemRegion.cpp \
	PathDiagnostic.cpp \
	PlistDiagnostics.cpp \
	ProgramState.cpp \
	RangeConstraintManager.cpp \
	RangedConstraintManager.cpp \
	RegionStore.cpp \
	SimpleConstraintManager.cpp \
	SimpleSValBuilder.cpp \
	Store.cpp \
	SubEngine.cpp \
	SValBuilder.cpp \
	SVals.cpp \
	SymbolManager.cpp \
	WorkList.cpp \
	Z3ConstraintManager.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
