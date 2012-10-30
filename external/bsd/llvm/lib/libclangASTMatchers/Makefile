#	$NetBSD: Makefile,v 1.1.4.2 2012/10/30 18:55:53 yamt Exp $

LIB=	clangASTMatchers

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/ASTMatchers

SRCS+=	ASTMatchFinder.cpp \
	ASTMatchersInternal.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
