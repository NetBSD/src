#	$NetBSD: Makefile,v 1.1.6.1 2012/04/17 00:03:34 yamt Exp $

LIB=	clangRewrite

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Rewrite

SRCS+=	DeltaTree.cpp \
	FixItRewriter.cpp \
	FrontendActions.cpp \
	HTMLPrint.cpp \
	HTMLRewrite.cpp \
	RewriteMacros.cpp \
	RewriteModernObjC.cpp \
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
