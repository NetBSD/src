#	$NetBSD: Makefile,v 1.1.2.2 2013/06/23 06:28:25 tls Exp $

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
