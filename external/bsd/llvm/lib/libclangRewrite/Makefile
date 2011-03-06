#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:53 bouyer Exp $

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
