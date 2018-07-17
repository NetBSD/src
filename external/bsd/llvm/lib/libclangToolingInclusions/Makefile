#	$NetBSD: Makefile,v 1.1 2018/07/17 18:55:31 joerg Exp $

LIB=	clangToolingInclusions

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/Inclusions

SRCS+=	HeaderIncludes.cpp \
	IncludeStyle.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
