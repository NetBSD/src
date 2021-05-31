#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:13 cjep Exp $

LIB=	clangTesting

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Testing

SRCS+=	CommandLineArgs.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
