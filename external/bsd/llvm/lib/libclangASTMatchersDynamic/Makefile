#	$NetBSD: Makefile,v 1.1.8.2 2014/05/22 16:14:45 yamt Exp $

LIB=	clangASTMatchersDynamic

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/ASTMatchers/Dynamic

SRCS+=	Diagnostics.cpp \
	Parser.cpp \
	Registry.cpp \
	VariantValue.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
