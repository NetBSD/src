#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 21:50:57 christos Exp $

LIB=	clangASTDiff

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/ASTDiff

SRCS+=	ASTDiff.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
