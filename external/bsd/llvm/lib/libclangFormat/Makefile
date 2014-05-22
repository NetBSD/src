#	$NetBSD: Makefile,v 1.4.4.2 2014/05/22 16:14:46 yamt Exp $

LIB=	clangFormat

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Format

SRCS+=	BreakableToken.cpp \
	ContinuationIndenter.cpp \
	Format.cpp \
	FormatToken.cpp \
	TokenAnnotator.cpp \
	UnwrappedLineParser.cpp \
	WhitespaceManager.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
