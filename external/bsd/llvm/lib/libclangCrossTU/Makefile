#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 21:50:56 christos Exp $

LIB=	clangCrossTU

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CrossTU

SRCS+=	CrossTranslationUnit.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
