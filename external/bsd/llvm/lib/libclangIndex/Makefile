#	$NetBSD: Makefile,v 1.6 2017/01/11 12:02:32 joerg Exp $

LIB=	clangIndex

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Index

SRCS+=	CodegenNameGenerator.cpp \
	CommentToXML.cpp \
	IndexBody.cpp \
	IndexDecl.cpp \
	IndexingAction.cpp \
	IndexingContext.cpp \
	IndexSymbol.cpp \
	IndexTypeSourceInfo.cpp \
	USRGeneration.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
