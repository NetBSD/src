#	$NetBSD: Makefile,v 1.7 2017/08/01 20:07:42 joerg Exp $

LIB=	clangFormat

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Format

SRCS+=	AffectedRangeManager.cpp \
	BreakableToken.cpp \
	ContinuationIndenter.cpp \
	Format.cpp \
	FormatToken.cpp \
	FormatTokenLexer.cpp \
	NamespaceEndCommentsFixer.cpp \
	SortJavaScriptImports.cpp \
	TokenAnalyzer.cpp \
	TokenAnnotator.cpp \
	UnwrappedLineFormatter.cpp \
	UnwrappedLineParser.cpp \
	UsingDeclarationsSorter.cpp \
	WhitespaceManager.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
