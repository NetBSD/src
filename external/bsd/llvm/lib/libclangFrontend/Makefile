#	$NetBSD: Makefile,v 1.7 2011/11/03 17:52:54 joerg Exp $

LIB=	clangFrontend

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Frontend

SRCS+=	ASTConsumers.cpp \
	ASTMerge.cpp \
	ASTUnit.cpp \
	CacheTokens.cpp \
	CompilerInstance.cpp \
	CompilerInvocation.cpp \
	CreateInvocationFromCommandLine.cpp \
	DependencyFile.cpp \
	FrontendAction.cpp \
	FrontendActions.cpp \
	FrontendOptions.cpp \
	HeaderIncludeGen.cpp \
	InitHeaderSearch.cpp \
	InitPreprocessor.cpp \
	LangStandards.cpp \
	LogDiagnosticPrinter.cpp \
	MultiplexConsumer.cpp \
	PrintPreprocessedOutput.cpp \
	SerializedDiagnosticPrinter.cpp \
	TextDiagnostic.cpp \
	TextDiagnosticBuffer.cpp \
	TextDiagnosticPrinter.cpp \
	VerifyDiagnosticConsumer.cpp \
	Warnings.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
