#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:13 cjep Exp $

LIB=	clangToolingCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Core

SRCS+=	Diagnostic.cpp \
	Replacement.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
