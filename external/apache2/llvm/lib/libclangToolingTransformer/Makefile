#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:17 joerg Exp $

LIB=	clangToolingTransformer

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Transformer

SRCS+=	RangeSelector.cpp \
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
