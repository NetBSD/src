#	$NetBSD: Makefile,v 1.3 2012/06/16 17:01:04 joerg Exp $

LIB=	clangRewrite

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Rewrite

SRCS+=	DeltaTree.cpp \
	FixItRewriter.cpp \
	FrontendActions.cpp \
	HTMLPrint.cpp \
	HTMLRewrite.cpp \
	InclusionRewriter.cpp \
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
