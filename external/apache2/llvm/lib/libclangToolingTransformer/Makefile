#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:14 cjep Exp $

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
