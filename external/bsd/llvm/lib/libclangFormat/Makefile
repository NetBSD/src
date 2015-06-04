#	$NetBSD: Makefile,v 1.4.6.1 2015/06/04 20:05:45 snj Exp $

LIB=	clangFormat

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Format

SRCS+=	BreakableToken.cpp \
	ContinuationIndenter.cpp \
	Format.cpp \
	FormatToken.cpp \
	TokenAnnotator.cpp \
	UnwrappedLineFormatter.cpp \
	UnwrappedLineParser.cpp \
	WhitespaceManager.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
