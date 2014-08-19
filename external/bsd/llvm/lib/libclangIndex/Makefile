#	$NetBSD: Makefile,v 1.5.6.2 2014/08/19 23:51:26 tls Exp $

LIB=	clangIndex

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Index

SRCS+=	CommentToXML.cpp \
	USRGeneration.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
