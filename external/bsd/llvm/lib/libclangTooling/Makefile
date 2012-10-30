#	$NetBSD: Makefile,v 1.3.2.2 2012/10/30 18:55:55 yamt Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	ArgumentsAdjusters.cpp \
	CommonOptionsParser.cpp \
	CompilationDatabase.cpp \
	FileMatchTrie.cpp \
	JSONCompilationDatabase.cpp \
	Refactoring.cpp \
	RefactoringCallbacks.cpp \
	Tooling.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
