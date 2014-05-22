#	$NetBSD: Makefile,v 1.1.6.3 2014/05/22 16:14:46 yamt Exp $

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
