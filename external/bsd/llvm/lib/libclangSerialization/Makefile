#	$NetBSD: Makefile,v 1.3.2.2 2014/05/22 16:14:46 yamt Exp $

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
	GeneratePCH.cpp \
	GlobalModuleIndex.cpp \
	Module.cpp \
	ModuleManager.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
