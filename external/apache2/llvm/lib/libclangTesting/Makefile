#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:57 joerg Exp $

LIB=	clangTesting

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Testing

SRCS+=	CommandLineArgs.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
