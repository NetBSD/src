#	$NetBSD: Makefile,v 1.9 2017/01/11 12:02:32 joerg Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	ArgumentsAdjusters.cpp \
	CommonOptionsParser.cpp \
	CompilationDatabase.cpp \
	FileMatchTrie.cpp \
	FixIt.cpp \
	JSONCompilationDatabase.cpp \
	Refactoring.cpp \
	RefactoringCallbacks.cpp \
	Tooling.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
