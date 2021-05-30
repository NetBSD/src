#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:56 joerg Exp $

LIB=	clangIndexSerialization

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/IndexSerialization

SRCS+=	SerializablePathCollection.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
