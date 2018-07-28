#	$NetBSD: Makefile,v 1.1.2.2 2018/07/28 04:37:14 pgoyette Exp $

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
