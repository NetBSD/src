#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:55 joerg Exp $

LIB=	clangAPINotes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/APINotes

SRCS+=	APINotesTypes.cpp \
	APINotesYAMLCompiler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
