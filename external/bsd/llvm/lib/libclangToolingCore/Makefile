#	$NetBSD: Makefile,v 1.2 2016/02/27 21:47:36 joerg Exp $

LIB=	clangToolingCore

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Core

SRCS+=	Lookup.cpp \
	Replacement.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
