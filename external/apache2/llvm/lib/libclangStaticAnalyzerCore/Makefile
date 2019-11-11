#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:16 joerg Exp $

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
	CommonBugCategories.cpp \
	ConstraintManager.cpp \
	CoreEngine.cpp \
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
	IssueHash.cpp \
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
	SubEngine.cpp \
	SValBuilder.cpp \
	SVals.cpp \
	SymbolManager.cpp \
	WorkList.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
