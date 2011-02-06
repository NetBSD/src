#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:10 joerg Exp $

LIB=	clangSerialization

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Serialization

SRCS+=	GeneratePCH.cpp \
	ASTCommon.cpp \
	ASTReader.cpp \
	ASTReaderDecl.cpp \
	ASTReaderStmt.cpp \
	ASTWriter.cpp \
	ASTWriterDecl.cpp \
	ASTWriterStmt.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
