#	$NetBSD: Makefile,v 1.10 2018/07/17 18:55:30 joerg Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	ArgumentsAdjusters.cpp \
	AllTUsExecution.cpp \
	CommonOptionsParser.cpp \
	CompilationDatabase.cpp \
	Execution.cpp \
	FileMatchTrie.cpp \
	FixIt.cpp \
	InterpolatingCompilationDatabase.cpp \
	JSONCompilationDatabase.cpp \
	RefactoringCallbacks.cpp \
	Refactoring.cpp \
	StandaloneExecution.cpp \
	Tooling.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
