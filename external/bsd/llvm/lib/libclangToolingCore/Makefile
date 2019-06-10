#	$NetBSD: Makefile,v 1.3.14.1 2019/06/10 21:50:57 christos Exp $

LIB=	clangToolingCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Core

SRCS+=	Diagnostic.cpp \
	Lookup.cpp \
	Replacement.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
