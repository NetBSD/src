#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:56 joerg Exp $

LIB=	clangAnalysis

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Analysis

SRCS+=	AnalysisDeclContext.cpp \
	BodyFarm.cpp \
	CalledOnceCheck.cpp \
	CallGraph.cpp \
	CFG.cpp \
	CFGReachabilityAnalysis.cpp \
	CFGStmtMap.cpp \
	CloneDetection.cpp \
	CocoaConventions.cpp \
	CodeInjector.cpp \
	ConstructionContext.cpp \
	Consumed.cpp \
	Dominators.cpp \
	ExprMutationAnalyzer.cpp \
	IssueHash.cpp \
	LiveVariables.cpp \
	MacroExpansionContext.cpp \
	ObjCNoReturn.cpp \
	PathDiagnostic.cpp \
	PostOrderCFGView.cpp \
	ProgramPoint.cpp \
	ReachableCode.cpp \
	RetainSummaryManager.cpp \
	ThreadSafetyCommon.cpp \
	ThreadSafety.cpp \
	ThreadSafetyLogical.cpp \
	ThreadSafetyTIL.cpp \
	UninitializedValues.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
