#	$NetBSD: Makefile,v 1.2 2011/06/01 14:13:42 joerg Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	ASTMatcher.cpp \
	JsonCompileCommandLineDatabase.cpp \
	Tooling.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
