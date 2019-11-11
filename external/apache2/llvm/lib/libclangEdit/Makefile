#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:15 joerg Exp $

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
