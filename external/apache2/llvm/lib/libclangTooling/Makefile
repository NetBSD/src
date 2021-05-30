#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:57 joerg Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	ArgumentsAdjusters.cpp \
	AllTUsExecution.cpp \
	CommonOptionsParser.cpp \
	CompilationDatabase.cpp \
	Execution.cpp \
	ExpandResponseFilesCompilationDatabase.cpp \
	FileMatchTrie.cpp \
	FixIt.cpp \
	GuessTargetAndModeCompilationDatabase.cpp \
	InterpolatingCompilationDatabase.cpp \
	JSONCompilationDatabase.cpp \
	NodeIntrospection.cpp \
	RefactoringCallbacks.cpp \
	Refactoring.cpp \
	StandaloneExecution.cpp \
	Tooling.cpp

clang/Tooling/NodeIntrospection.inc: ${CLANG_SRCDIR}/lib/Tooling
	mkdir -p ${.TARGET:H}
	${CAT} < ${CLANG_SRCDIR}/lib/Tooling/EmptyNodeIntrospection.inc.in > ${.TARGET}

DPSRCS+=	clang/Tooling/NodeIntrospection.inc
CLEANFILES+=	clang/Tooling/NodeIntrospection.inc

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
