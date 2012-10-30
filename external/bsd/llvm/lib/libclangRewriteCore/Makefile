#	$NetBSD: Makefile,v 1.1.2.2 2012/10/30 18:55:54 yamt Exp $

LIB=	clangRewriteCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Rewrite/Core

SRCS+=	DeltaTree.cpp \
	HTMLRewrite.cpp \
	RewriteRope.cpp \
	Rewriter.cpp \
	TokenRewriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
