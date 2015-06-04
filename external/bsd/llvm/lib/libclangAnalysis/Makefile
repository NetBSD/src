#	$NetBSD: Makefile,v 1.12.2.1 2015/06/04 20:05:45 snj Exp $

LIB=	clangAnalysis

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Analysis

SRCS+=	AnalysisDeclContext.cpp \
	BodyFarm.cpp \
	CFG.cpp \
	CFGReachabilityAnalysis.cpp \
	CFGStmtMap.cpp \
	CallGraph.cpp \
	CocoaConventions.cpp \
	CodeInjector.cpp \
	Consumed.cpp \
	Dominators.cpp \
	FormatString.cpp \
	LiveVariables.cpp \
	ObjCNoReturn.cpp \
	PostOrderCFGView.cpp \
	ProgramPoint.cpp \
	PrintfFormatString.cpp \
	PseudoConstantAnalysis.cpp \
	ReachableCode.cpp \
	ScanfFormatString.cpp \
	ThreadSafety.cpp \
	ThreadSafetyCommon.cpp \
	ThreadSafetyLogical.cpp \
	ThreadSafetyTIL.cpp \
	UninitializedValues.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
