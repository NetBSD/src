#	$NetBSD: Makefile,v 1.5 2011/09/12 13:32:59 joerg Exp $

LIB=	clangStaticAnalyzerCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Core

SRCS+=	AggExprVisitor.cpp \
	AnalysisManager.cpp \
	BasicConstraintManager.cpp \
	BasicValueFactory.cpp \
	BlockCounter.cpp \
	BugReporter.cpp \
	BugReporterVisitors.cpp \
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
	HTMLDiagnostics.cpp \
	MemRegion.cpp \
	ObjCMessage.cpp \
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
	SymbolManager.cpp \
	TextPathDiagnostics.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
