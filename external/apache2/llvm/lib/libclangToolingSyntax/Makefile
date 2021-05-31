#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:13 cjep Exp $

LIB=	clangToolingSyntax

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Syntax

SRCS+=	BuildTree.cpp \
	ComputeReplacements.cpp \
	Mutations.cpp \
	Nodes.cpp \
	Synthesis.cpp \
	Tokens.cpp \
	Tree.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
