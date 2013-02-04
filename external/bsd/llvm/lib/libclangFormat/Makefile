#	$NetBSD: Makefile,v 1.2 2013/02/04 15:00:36 joerg Exp $

LIB=	clangFormat

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Format

SRCS+=	Format.cpp \
	TokenAnnotator.cpp \
	UnwrappedLineParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
