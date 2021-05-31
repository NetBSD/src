#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:11 cjep Exp $

LIB=	clangASTMatchers

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/ASTMatchers

SRCS+=	ASTMatchFinder.cpp \
	ASTMatchersInternal.cpp \
	GtestMatchers.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
