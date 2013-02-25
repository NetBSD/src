#	$NetBSD: Makefile,v 1.2.4.2 2013/02/25 00:26:41 tls Exp $

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
