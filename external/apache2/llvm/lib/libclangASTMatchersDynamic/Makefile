#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:14 joerg Exp $

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
