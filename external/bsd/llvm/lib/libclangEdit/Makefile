#	$NetBSD: Makefile,v 1.1.2.2 2012/04/17 00:03:34 yamt Exp $

LIB=	clangEdit

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Edit

SRCS+=	Commit.cpp \
	EditedSource.cpp \
	RewriteObjCFoundationAPI.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
