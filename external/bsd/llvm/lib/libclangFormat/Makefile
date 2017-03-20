#	$NetBSD: Makefile,v 1.5.2.1 2017/03/20 06:56:02 pgoyette Exp $

LIB=	clangFormat

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Format

SRCS+=	AffectedRangeManager.cpp \
	BreakableToken.cpp \
	Comments.cpp \
	ContinuationIndenter.cpp \
	Format.cpp \
	FormatToken.cpp \
	FormatTokenLexer.cpp \
	SortJavaScriptImports.cpp \
	TokenAnalyzer.cpp \
	TokenAnnotator.cpp \
	UnwrappedLineFormatter.cpp \
	UnwrappedLineParser.cpp \
	WhitespaceManager.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
