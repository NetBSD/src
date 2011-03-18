#	$NetBSD: Makefile,v 1.2 2011/03/18 23:32:04 joerg Exp $

LIB=	clangSerialization

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Serialization

SRCS+=	ASTCommon.cpp \
	ASTReader.cpp \
	ASTReaderDecl.cpp \
	ASTReaderStmt.cpp \
	ASTWriter.cpp \
	ASTWriterDecl.cpp \
	ASTWriterStmt.cpp \
	ChainedIncludesSource.cpp \
	GeneratePCH.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
