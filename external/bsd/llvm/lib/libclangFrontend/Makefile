#	$NetBSD: Makefile,v 1.13 2014/08/10 17:34:22 joerg Exp $

LIB=	clangFrontend

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Frontend

SRCS+=	ASTConsumers.cpp \
	ASTMerge.cpp \
	ASTUnit.cpp \
	CacheTokens.cpp \
	ChainedDiagnosticConsumer.cpp \
	ChainedIncludesSource.cpp \
	CompilerInstance.cpp \
	CompilerInvocation.cpp \
	CreateInvocationFromCommandLine.cpp \
	DependencyFile.cpp \
	DependencyGraph.cpp \
	DiagnosticRenderer.cpp \
	FrontendAction.cpp \
	FrontendActions.cpp \
	FrontendOptions.cpp \
	HeaderIncludeGen.cpp \
	InitHeaderSearch.cpp \
	InitPreprocessor.cpp \
	LangStandards.cpp \
	LayoutOverrideSource.cpp \
	LogDiagnosticPrinter.cpp \
	ModuleDependencyCollector.cpp \
	MultiplexConsumer.cpp \
	PrintPreprocessedOutput.cpp \
	SerializedDiagnosticPrinter.cpp \
	TextDiagnostic.cpp \
	TextDiagnosticBuffer.cpp \
	TextDiagnosticPrinter.cpp \
	VerifyDiagnosticConsumer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
