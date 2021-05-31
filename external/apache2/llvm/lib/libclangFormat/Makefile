#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:12 cjep Exp $

LIB=	clangFormat

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Format

SRCS+=	AffectedRangeManager.cpp \
	BreakableToken.cpp \
	ContinuationIndenter.cpp \
	Format.cpp \
	FormatToken.cpp \
	FormatTokenLexer.cpp \
	MacroExpander.cpp \
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
