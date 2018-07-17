#	$NetBSD: Makefile,v 1.1 2018/07/17 18:55:30 joerg Exp $

LIB=	clangASTDiff

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/ASTDiff

SRCS+=	ASTDiff.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
