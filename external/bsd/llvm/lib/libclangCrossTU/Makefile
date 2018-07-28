#	$NetBSD: Makefile,v 1.1.2.2 2018/07/28 04:37:13 pgoyette Exp $

LIB=	clangCrossTU

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CrossTU

SRCS+=	CrossTranslationUnit.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
