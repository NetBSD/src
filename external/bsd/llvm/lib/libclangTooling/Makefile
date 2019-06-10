#	$NetBSD: Makefile,v 1.9.14.1 2019/06/10 21:50:57 christos Exp $

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
