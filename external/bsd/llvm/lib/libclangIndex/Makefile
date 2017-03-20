#	$NetBSD: Makefile,v 1.5.10.1 2017/03/20 06:56:02 pgoyette Exp $

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
