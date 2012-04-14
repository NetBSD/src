#	$NetBSD: Makefile,v 1.4 2012/04/14 15:11:18 joerg Exp $

LIB=	clangTooling

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling

SRCS+=	CompilationDatabase.cpp \
	Tooling.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
