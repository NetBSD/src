#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:08 joerg Exp $

LIB=	clangLex

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Lex

SRCS+=	HeaderMap.cpp \
	HeaderSearch.cpp \
	Lexer.cpp \
	LiteralSupport.cpp \
	MacroArgs.cpp \
	MacroInfo.cpp \
	PPCaching.cpp \
	PPDirectives.cpp \
	PPExpressions.cpp \
	PPLexerChange.cpp \
	PPMacroExpansion.cpp \
	PTHLexer.cpp \
	Pragma.cpp \
	PreprocessingRecord.cpp \
	Preprocessor.cpp \
	PreprocessorLexer.cpp \
	ScratchBuffer.cpp \
	TokenConcatenation.cpp \
	TokenLexer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
