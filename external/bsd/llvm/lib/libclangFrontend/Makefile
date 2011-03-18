#	$NetBSD: Makefile,v 1.2 2011/03/18 23:32:04 joerg Exp $

LIB=	clangFrontend

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Frontend

SRCS+=	ASTConsumers.cpp \
	ASTMerge.cpp \
	ASTUnit.cpp \
	BoostConAction.cpp \
	CacheTokens.cpp \
	CompilerInstance.cpp \
	CompilerInvocation.cpp \
	DependencyFile.cpp \
	DiagChecker.cpp \
	FrontendAction.cpp \
	FrontendActions.cpp \
	FrontendOptions.cpp \
	HeaderIncludeGen.cpp \
	InitHeaderSearch.cpp \
	InitPreprocessor.cpp \
	LangStandards.cpp \
	MultiplexConsumer.cpp \
	PrintPreprocessedOutput.cpp \
	TextDiagnosticBuffer.cpp \
	TextDiagnosticPrinter.cpp \
	VerifyDiagnosticsClient.cpp \
	Warnings.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
