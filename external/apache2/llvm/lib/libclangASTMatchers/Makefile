#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:56 joerg Exp $

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
