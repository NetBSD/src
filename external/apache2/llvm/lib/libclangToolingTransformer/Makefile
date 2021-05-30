#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:58 joerg Exp $

LIB=	clangToolingTransformer

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Transformer

SRCS+=	Parsing.cpp \
	RangeSelector.cpp \
	RewriteRule.cpp \
	SourceCodeBuilders.cpp \
	SourceCode.cpp \
	Stencil.cpp \
	Transformer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
