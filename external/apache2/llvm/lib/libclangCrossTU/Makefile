#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:15 joerg Exp $

LIB=	clangCrossTU

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CrossTU

SRCS+=	CrossTranslationUnit.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
