#	$NetBSD: Makefile,v 1.6.2.1 2012/11/20 02:58:03 tls Exp $

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
