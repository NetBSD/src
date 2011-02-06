#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:09 joerg Exp $

LIB=	clangRewrite

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Rewrite

SRCS+=	DeltaTree.cpp \
	FixItRewriter.cpp \
	FrontendActions.cpp \
	HTMLPrint.cpp \
	HTMLRewrite.cpp \
	RewriteMacros.cpp \
	RewriteObjC.cpp \
	RewriteRope.cpp \
	RewriteTest.cpp \
	Rewriter.cpp \
	TokenRewriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
