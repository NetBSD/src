#	$NetBSD: Makefile,v 1.2 2012/02/28 17:09:31 joerg Exp $

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
