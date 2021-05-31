#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:12 cjep Exp $

LIB=	clangIndexSerialization

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/IndexSerialization

SRCS+=	SerializablePathCollection.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
