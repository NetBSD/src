#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:17 joerg Exp $

LIB=	clangToolingSyntax

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Syntax

SRCS+=	BuildTree.cpp \
	Nodes.cpp \
	Tokens.cpp \
	Tree.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
