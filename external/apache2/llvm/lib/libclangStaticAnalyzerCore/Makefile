#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:57 joerg Exp $

LIB=	clangStaticAnalyzerCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Core

SRCS+=	AnalysisManager.cpp \
	AnalyzerOptions.cpp \
	APSIntType.cpp \
	BasicValueFactory.cpp \
	BlockCounter.cpp \
	BugReporter.cpp \
	BugReporterVisitors.cpp \
	CallEvent.cpp \
	CheckerContext.cpp \
	Checker.cpp \
	CheckerHelpers.cpp \
	CheckerManager.cpp \
	CheckerRegistryData.cpp \
	CommonBugCategories.cpp \
	ConstraintManager.cpp \
	CoreEngine.cpp \
	DynamicExtent.cpp \
	DynamicType.cpp \
	Environment.cpp \
	ExplodedGraph.cpp \
	ExprEngineCallAndReturn.cpp \
	ExprEngineC.cpp \
	ExprEngine.cpp \
	ExprEngineCXX.cpp \
	ExprEngineObjC.cpp \
	FunctionSummary.cpp \
	HTMLDiagnostics.cpp \
	LoopUnrolling.cpp \
	LoopWidening.cpp \
	MemRegion.cpp \
	PlistDiagnostics.cpp \
	ProgramState.cpp \
	RangeConstraintManager.cpp \
	RangedConstraintManager.cpp \
	RegionStore.cpp \
	SarifDiagnostics.cpp \
	SimpleConstraintManager.cpp \
	SimpleSValBuilder.cpp \
	SMTConstraintManager.cpp \
	Store.cpp \
	SValBuilder.cpp \
	SVals.cpp \
	SymbolManager.cpp \
	TextDiagnostics.cpp \
	WorkList.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
