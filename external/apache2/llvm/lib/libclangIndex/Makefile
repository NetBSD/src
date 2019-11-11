#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:16 joerg Exp $

LIB=	clangIndex

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Index

SRCS+=	CommentToXML.cpp \
	FileIndexRecord.cpp \
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
