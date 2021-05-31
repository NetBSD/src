#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:11 cjep Exp $

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
