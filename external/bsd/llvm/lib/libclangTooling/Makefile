#	$NetBSD: Makefile,v 1.6 2012/08/09 14:37:07 joerg Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	ArgumentsAdjusters.cpp \
	CommandLineClangTool.cpp \
	CompilationDatabase.cpp \
	Refactoring.cpp \
	RefactoringCallbacks.cpp \
	Tooling.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
