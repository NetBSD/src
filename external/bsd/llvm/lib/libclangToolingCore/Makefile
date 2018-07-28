#	$NetBSD: Makefile,v 1.3.12.1 2018/07/28 04:37:14 pgoyette Exp $

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
