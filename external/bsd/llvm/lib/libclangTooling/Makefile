#	$NetBSD: Makefile,v 1.5 2012/06/16 17:01:04 joerg Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	ArgumentsAdjusters.cpp \
	CompilationDatabase.cpp \
	Refactoring.cpp \
	Tooling.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
