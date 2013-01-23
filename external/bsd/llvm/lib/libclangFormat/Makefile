#	$NetBSD: Makefile,v 1.1 2013/01/23 15:03:01 joerg Exp $

LIB=	clangFormat

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Format

SRCS+=	Format.cpp \
	UnwrappedLineParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
