#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:06 joerg Exp $

LIB=	clangAnalysis

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Analysis

SRCS+=	AnalysisContext.cpp \
	CFG.cpp \
	CFGStmtMap.cpp \
	CocoaConventions.cpp \
	FormatString.cpp \
	LiveVariables.cpp \
	PrintfFormatString.cpp \
	PseudoConstantAnalysis.cpp \
	ReachableCode.cpp \
	ScanfFormatString.cpp \
	UninitializedValues.cpp \
	UninitializedValuesV2.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
